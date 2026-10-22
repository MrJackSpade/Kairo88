// QUASI88's sound driver interface (snddrv.h) implemented with ymfm.
//
// QUASI88 emulates the sound boards' timers, status flags and ADPCM RAM itself;
// the driver only produces audio. Every register write is applied at the CPU
// cycle it happened: the chip is first rendered up to that moment. Once per
// frame QUASI88 calls xmame_sound_update(), which renders up to the present.
//
// Chips: sound board I is a YM2203 (OPN) at 3.9936 MHz, sound board II a
// YM2608 (OPNA) at 7.9872 MHz. The YM2608 rhythm section needs the chip's
// internal ROM, which is not distributed; instead the six rhythm instruments
// are played from the user's 2608_*.wav samples, as other PC-88 emulators do.
// The beeper (port 0x40: bit 5 gates a 2.4 kHz tone, bit 7 drives the
// speaker directly) is mixed in. The PCG-8100 board and drive-noise samples
// are not emulated.
//
// Output is stereo 16-bit at 48 kHz in a ring buffer the host drains with
// k88_audio_read().

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "ymfm_opn.h"

extern "C" {
#include "quasi88.h"
#include "getconf.h"
#include "file-op.h"
#include "snddrv.h"
#include "intr.h"
#include "pc88cpu.h"
#include "soundbd.h"
}

#include "kairo88_core.h"

namespace {

constexpr uint32_t kOutputRate = 48000;
constexpr uint32_t kRingFrames = kOutputRate;  // one second
constexpr uint32_t kOpnClock = 3993600;
constexpr uint32_t kOpnaClock = 7987200;
constexpr double kBeepHz = 2400.0;

// One of the YM2608's six rhythm instruments, played from a WAV sample.
struct RhythmVoice {
	std::vector<int16_t> samples;
	double step = 0;       // sample positions per native output sample
	double position = -1;  // < 0 when silent
	int level = 0;         // instrument level, 0..31 (31 loudest)
	bool left = true;
	bool right = true;
};

class Driver final : public ymfm::ymfm_interface {
public:
	void start(int board)
	{
		m_board = board;
		m_2203.reset();
		m_2608.reset();
		if (board == SOUND_II) {
			m_2608 = std::make_unique<ymfm::ym2608>(*this);
			m_2608->reset();
			m_native_rate = m_2608->sample_rate(kOpnaClock);
		} else {
			m_2203 = std::make_unique<ymfm::ym2203>(*this);
			m_2203->reset();
			m_native_rate = m_2203->sample_rate(kOpnClock);
		}
		m_adpcm_ram.assign(256 * 1024, 0);
		load_rhythm();
		m_last_states = now_states();
		m_native_due = 0;
		m_out_phase = 0;
		m_acc_left = m_acc_right = 0;
		m_acc_count = 0;
		m_beep_phase = 0;
		m_beep_enable = m_sing = false;
		m_rhythm_total = 0;
	}

	void reset()
	{
		if (m_2608) m_2608->reset();
		if (m_2203) m_2203->reset();
		for (auto &voice : m_rhythm) voice.position = -1;
	}

	void write(int bank, uint8_t reg, uint8_t data)
	{
		sync();
		if (m_2608) {
			m_2608->write(bank ? 2 : 0, reg);
			m_2608->write(bank ? 3 : 1, data);
			if (!bank) rhythm_write(reg, data);
		} else if (m_2203 && !bank) {
			m_2203->write(0, reg);
			m_2203->write(1, data);
		}
	}

	void beep(uint8_t port40)
	{
		sync();
		m_beep_enable = (port40 & 0x20) != 0;
		m_sing = (port40 & 0x80) != 0;
	}

	void sing(bool on)
	{
		sync();
		m_sing = on;
	}

	// Renders everything up to the present CPU cycle.
	void sync()
	{
		const int64_t now = now_states();
		const int64_t elapsed = now - m_last_states;
		m_last_states = now;
		if (elapsed <= 0 || m_native_rate == 0) return;
		const double cpu_hz = cpu_clock_mhz * 1000000.0;
		m_native_due += static_cast<double>(elapsed) * m_native_rate / cpu_hz;
		const auto samples = static_cast<uint32_t>(m_native_due);
		m_native_due -= samples;
		for (uint32_t i = 0; i < samples; ++i) render_one();
	}

	uint32_t read(int16_t *stereo, uint32_t frames)
	{
		const uint32_t count = std::min(frames, m_ring_count);
		for (uint32_t i = 0; i < count; ++i) {
			stereo[i * 2] = m_ring[m_ring_read * 2];
			stereo[i * 2 + 1] = m_ring[m_ring_read * 2 + 1];
			m_ring_read = (m_ring_read + 1) % kRingFrames;
		}
		m_ring_count -= count;
		return count;
	}

	uint32_t available() const { return m_ring_count; }

	uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address) override
	{
		if (type == ymfm::ACCESS_ADPCM_B && address < m_adpcm_ram.size()) return m_adpcm_ram[address];
		return 0;
	}

	void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override
	{
		if (type == ymfm::ACCESS_ADPCM_B && address < m_adpcm_ram.size()) m_adpcm_ram[address] = data;
	}

private:
	// CPU cycles since power-on: whole vsync periods plus the cycles into this one.
	static int64_t now_states()
	{
		return static_cast<int64_t>(quasi88_info_vsync_count()) * state_of_vsync + state_of_cpu + z80main_cpu.state0;
	}

	void render_one()
	{
		int32_t left = 0, right = 0;
		if (m_2608) {
			ymfm::ym2608::output_data out{};
			m_2608->generate(&out);
			left = out.data[0] + out.data[2];
			right = out.data[1] + out.data[2];
		} else if (m_2203) {
			ymfm::ym2203::output_data out{};
			m_2203->generate(&out);
			const int32_t ssg = out.data[1] + out.data[2] + out.data[3];
			left = right = out.data[0] + ssg;
		}
		if (m_2608) rhythm_mix(left, right);

		const bool beep_high = m_beep_enable && m_beep_phase < 0.5;
		m_beep_phase += kBeepHz / m_native_rate;
		if (m_beep_phase >= 1.0) m_beep_phase -= 1.0;
		if (beep_high || m_sing) {
			left += 6000;
			right += 6000;
		}

		// Box-filter the native-rate stream down to the output rate.
		m_acc_left += left;
		m_acc_right += right;
		m_acc_count++;
		m_out_phase += kOutputRate;
		if (m_out_phase >= m_native_rate) {
			m_out_phase -= m_native_rate;
			push(static_cast<int32_t>(m_acc_left / m_acc_count), static_cast<int32_t>(m_acc_right / m_acc_count));
			m_acc_left = m_acc_right = 0;
			m_acc_count = 0;
		}
	}

	void push(int32_t left, int32_t right)
	{
		if (m_ring_count == kRingFrames) {  // host stopped reading: drop the oldest frame
			m_ring_read = (m_ring_read + 1) % kRingFrames;
			m_ring_count--;
		}
		const uint32_t write = (m_ring_read + m_ring_count) % kRingFrames;
		m_ring[write * 2] = static_cast<int16_t>(std::clamp(left, -32768, 32767));
		m_ring[write * 2 + 1] = static_cast<int16_t>(std::clamp(right, -32768, 32767));
		m_ring_count++;
	}

	void rhythm_write(uint8_t reg, uint8_t data)
	{
		if (reg == 0x10) {
			for (int i = 0; i < 6; ++i) {
				if (!(data & (1 << i))) continue;
				m_rhythm[i].position = (data & 0x80) ? -1 : 0;
			}
		} else if (reg == 0x11) {
			m_rhythm_total = data & 0x3f;
		} else if (reg >= 0x18 && reg <= 0x1d) {
			RhythmVoice &voice = m_rhythm[reg - 0x18];
			voice.left = (data & 0x80) != 0;
			voice.right = (data & 0x40) != 0;
			voice.level = data & 0x1f;
		}
	}

	void rhythm_mix(int32_t &left, int32_t &right)
	{
		for (RhythmVoice &voice : m_rhythm) {
			if (voice.position < 0 || voice.samples.empty()) continue;
			const auto index = static_cast<size_t>(voice.position);
			if (index >= voice.samples.size()) {
				voice.position = -1;
				continue;
			}
			// Total level and instrument level attenuate in 0.75 dB steps.
			const double db = -0.75 * ((63 - m_rhythm_total) + (31 - voice.level));
			const auto sample = static_cast<int32_t>(voice.samples[index] * std::pow(10.0, db / 20.0));
			if (voice.left) left += sample;
			if (voice.right) right += sample;
			voice.position += voice.step;
		}
	}

	void load_rhythm()
	{
		static const char *const names[6] = {"2608_bd.wav", "2608_sd.wav", "2608_top.wav",
			"2608_hh.wav", "2608_tom.wav", "2608_rim.wav"};
		for (int i = 0; i < 6; ++i) {
			m_rhythm[i] = RhythmVoice{};
			uint32_t rate = 0;
			const std::string path = std::string(osd_dir_rom()) + "/" + names[i];
			if (read_wav(path, m_rhythm[i].samples, rate)) {
				m_rhythm[i].step = static_cast<double>(rate) / m_native_rate;
			} else {
				m_rhythm[i].samples.clear();
			}
		}
	}

	// Reads a PCM WAV (8- or 16-bit, mono or stereo) as mono 16-bit samples.
	static bool read_wav(const std::string &path, std::vector<int16_t> &samples, uint32_t &rate)
	{
		FILE *file = std::fopen(path.c_str(), "rb");
		if (!file) return false;
		std::vector<uint8_t> bytes;
		uint8_t chunk[4096];
		size_t got;
		while ((got = std::fread(chunk, 1, sizeof chunk, file)) > 0) bytes.insert(bytes.end(), chunk, chunk + got);
		std::fclose(file);
		auto u16 = [&](size_t at) { return static_cast<uint32_t>(bytes[at] | bytes[at + 1] << 8); };
		auto u32 = [&](size_t at) { return u16(at) | u16(at + 2) << 16; };
		if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) || std::memcmp(bytes.data() + 8, "WAVE", 4)) {
			std::fprintf(stderr, "kairo88: %s is not a WAV file\n", path.c_str());
			return false;
		}
		uint32_t channels = 0, bits = 0;
		for (size_t at = 12; at + 8 <= bytes.size();) {
			const uint32_t size = u32(at + 4);
			const size_t body = at + 8;
			if (body + size > bytes.size()) break;
			if (!std::memcmp(bytes.data() + at, "fmt ", 4) && size >= 16) {
				if (u16(body) != 1) {
					std::fprintf(stderr, "kairo88: %s is not PCM\n", path.c_str());
					return false;
				}
				channels = u16(body + 2);
				rate = u32(body + 4);
				bits = u16(body + 14);
			} else if (!std::memcmp(bytes.data() + at, "data", 4) && channels && (bits == 8 || bits == 16)) {
				const size_t frame_bytes = channels * bits / 8;
				for (size_t frame = body; frame + frame_bytes <= body + size; frame += frame_bytes) {
					samples.push_back(bits == 16 ? static_cast<int16_t>(u16(frame))
							: static_cast<int16_t>((bytes[frame] - 128) << 8));
				}
				return !samples.empty();
			}
			at = body + size + (size & 1);
		}
		std::fprintf(stderr, "kairo88: %s has no usable PCM data\n", path.c_str());
		return false;
	}

	int m_board = SOUND_I;
	std::unique_ptr<ymfm::ym2203> m_2203;
	std::unique_ptr<ymfm::ym2608> m_2608;
	std::vector<uint8_t> m_adpcm_ram;
	uint32_t m_native_rate = 0;
	int64_t m_last_states = 0;
	double m_native_due = 0;
	uint32_t m_out_phase = 0;
	int64_t m_acc_left = 0, m_acc_right = 0;
	int32_t m_acc_count = 0;
	double m_beep_phase = 0;
	bool m_beep_enable = false;
	bool m_sing = false;
	std::array<RhythmVoice, 6> m_rhythm{};
	int m_rhythm_total = 0;
	std::array<int16_t, kRingFrames * 2> m_ring{};
	uint32_t m_ring_read = 0;
	uint32_t m_ring_count = 0;
};

Driver g_driver;
bool g_started = false;
int g_master_volume = 0;
int g_mixer_volume[XMAME_MIXER_END] = {100, 100, 60, 60, 100, 100, 100, 100};

}  // namespace

extern "C" {

// Options --------------------------------------------------------------------

int xmame_config_init(void) { return 0; }
void xmame_config_exit(void) {}
const T_CONFIG_TABLE *xmame_config_get_opt_tbl(void) { return nullptr; }
void xmame_config_show_option(FILE *fp) { (void) fp; }
int xmame_config_check_option(char *arg1, char *arg2, int priority)
{
	(void) arg1;
	(void) arg2;
	(void) priority;
	return 0;
}
int xmame_config_save_option(void (*real_write)(const char *, const char *))
{
	(void) real_write;
	return 0;
}
T_SNDDRV_CONFIG *xmame_config_get_sndopt_tbl(void) { return nullptr; }

// State ----------------------------------------------------------------------

int xmame_has_sound(void) { return TRUE; }
int xmame_has_audiodevice(void) { return TRUE; }
int xmame_has_mastervolume(void) { return FALSE; }

int xmame_sound_start(void)
{
	g_driver.start(sound_board);
	g_started = true;
	return TRUE;
}

void xmame_sound_update(void)
{
	if (g_started) g_driver.sync();
}

void xmame_update_video_and_audio(void) {}
void xmame_sound_stop(void) { g_started = false; }
void xmame_sound_suspend(void) {}
void xmame_sound_resume(void) {}
void xmame_sound_reset(void)
{
	if (g_started) g_driver.reset();
}

// Devices --------------------------------------------------------------------

static uint8_t g_reg = 0, g_reg2 = 0;

byte xmame_dev_sound_in_data(void) { return 0xff; }
byte xmame_dev_sound_in_status(void) { return 0; }
void xmame_dev_sound_out_reg(byte data) { g_reg = data; }
void xmame_dev_sound_out_data(byte data)
{
	if (g_started) g_driver.write(0, g_reg, data);
}
byte xmame_dev_sound2_in_data(void) { return 0xff; }
byte xmame_dev_sound2_in_status(void) { return 0; }
void xmame_dev_sound2_out_reg(byte data) { g_reg2 = data; }
void xmame_dev_sound2_out_data(byte data)
{
	if (g_started) g_driver.write(1, g_reg2, data);
}
void xmame_dev_beep_out_data(byte data)
{
	if (g_started) g_driver.beep(data);
}
void xmame_dev_beep_cmd_sing(byte flag)
{
	if (g_started) g_driver.sing(flag != 0);
}
void xmame_dev_pcg_out_gate(byte data) { (void) data; }
void xmame_dev_pcg_out_counter0(byte data) { (void) data; }
void xmame_dev_pcg_out_counter1(byte data) { (void) data; }
void xmame_dev_pcg_out_counter2(byte data) { (void) data; }
void xmame_dev_pcg_out_control(byte data) { (void) data; }
void xmame_dev_sample_motoron(void) {}
void xmame_dev_sample_motoroff(void) {}
void xmame_dev_sample_headdown(void) {}
void xmame_dev_sample_headup(void) {}
void xmame_dev_sample_seek(void) {}
void xmame_dev_sound_timer_over(int timer) { (void) timer; }

// Settings shown in QUASI88's own menus, which Kairo88 does not display -----

int xmame_cfg_get_mastervolume(void) { return g_master_volume; }
void xmame_cfg_set_mastervolume(int vol) { g_master_volume = vol; }
int xmame_cfg_get_mixer_volume(int ch) { return ch >= 0 && ch < XMAME_MIXER_END ? g_mixer_volume[ch] : 0; }
void xmame_cfg_set_mixer_volume(int ch, int level)
{
	if (ch >= 0 && ch < XMAME_MIXER_END) g_mixer_volume[ch] = level;
}
int xmame_cfg_get_use_fmgen(void) { return FALSE; }
int xmame_cfg_set_use_fmgen(int enable) { (void) enable; return FALSE; }
int xmame_cfg_get_use_samples(void) { return FALSE; }
int xmame_cfg_set_use_samples(int enable) { (void) enable; return FALSE; }
int xmame_cfg_get_sample_freq(void) { return kOutputRate; }
int xmame_cfg_set_sample_freq(int freq) { (void) freq; return kOutputRate; }

int xmame_wavout_open(const char *filename) { (void) filename; return FALSE; }
int xmame_wavout_opened(void) { return FALSE; }
void xmame_wavout_close(void) {}
int xmame_wavout_damaged(void) { return FALSE; }

const char *xmame_version_mame(void) { return ""; }
const char *xmame_version_fmgen(void) { return ""; }

// Kairo88 core API -------------------------------------------------------------

int k88_audio_rate(void) { return static_cast<int>(kOutputRate); }

int k88_audio_read(int16_t *stereo, int max_frames)
{
	return max_frames > 0 ? static_cast<int>(g_driver.read(stereo, static_cast<uint32_t>(max_frames))) : 0;
}

int k88_audio_available(void) { return static_cast<int>(g_driver.available()); }

}  // extern "C"
