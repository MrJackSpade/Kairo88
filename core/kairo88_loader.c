/* A game's startup commands: types its lines at the machine's prompts, as a player would, after k88_boot. */
#include <stdio.h>
#include <string.h>

#include "kairo88_core.h"

/* QUASI88 key codes (pc88/keyboard.h). */
enum { KEY_SHIFT = 152, KEY_CTRL = 153, KEY_RETURN = 145, KEY_F1 = 165 };
enum { MAX_LINES = 8, MAX_LINE = 48, MAX_EVENTS = 4096, SETTLE_FRAMES = 30 };

/* One key change and the frames to wait after it; BASIC's keyboard scan needs each change to last. */
struct event { int key; int down; int wait; };

static char lines[MAX_LINES][MAX_LINE];
static int line_count, line_next;
static struct event events[MAX_EVENTS];
static int event_count, event_next, wait_frames, settle, active;

static void push(int key, int down, int wait)
{
	if (event_count < MAX_EVENTS) events[event_count++] = (struct event) {key, down, wait};
}

static void tap(int key, int shifted)
{
	if (shifted) push(KEY_SHIFT, 1, 2);
	push(key, 1, 5);
	if (shifted) push(key, 0, 2);
	push(shifted ? KEY_SHIFT : key, 0, 5);
}

/* The PC-88 key, and whether Shift goes with it, for a character of a line; -1 for none. QUASI88's
 * code for a printable character is the character, on the key that carries it (pc88/keyboard.c). */
static int key_of(char c, int *shifted)
{
	/* The characters a JIS key gives with Shift. */
	static const char shifted_chars[] = "!\"#$%&'()=~|`{+*}<>?_";
	*shifted = 0;
	/* Letters with Shift are upper case with Caps off, and lower case without (disk file names). */
	if (c >= 'A' && c <= 'Z') {
		*shifted = 1;
		return c;
	}
	if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
	if (c < ' ' || c > '~') return -1;
	*shifted = strchr(shifted_chars, c) != NULL;
	return c;
}

/* A line and Return; {CTRL-x} is Ctrl with x, {F1} to {F5} a function key. */
static int queue_line(const char *line)
{
	const char *c;
	for (c = line; *c; ++c) {
		int shifted, key;
		if (*c == '{') {
			const char *end = strchr(c, '}');
			if (!end) return -1;
			if (strncmp(c, "{CTRL-", 6) == 0 && end == c + 7) {
				push(KEY_CTRL, 1, 2);
				tap(c[6], 0);
				push(KEY_CTRL, 0, 5);
			} else if (c[1] == 'F' && c[2] >= '1' && c[2] <= '5' && end == c + 3) {
				tap(KEY_F1 + (c[2] - '1'), 0);
			} else {
				return -1;
			}
			c = end;
			continue;
		}
		key = key_of(*c, &shifted);
		if (key < 0) return -1;
		tap(key, shifted);
	}
	tap(KEY_RETURN, 0);
	return 0;
}

/* The screen's last line that is not the function key row, with the cursor and any byte that is
 * not ASCII (shown as '?') removed. */
static void last_line(char *out, int size)
{
	static char text[8192];
	char *line, *next, *found = NULL;
	k88_screen_text(text, sizeof(text));
	for (line = text; *line; line = next) {
		next = strchr(line, '\n');
		if (next) *next++ = '\0'; else next = line + strlen(line);
		if (strstr(line, "auto") && strstr(line, "list") && strstr(line, "run")) continue;
		if (strspn(line, " ?") != strlen(line)) found = line;
	}
	int written = 0;
	for (line = found; line && *line && written < size - 1; ++line)
		if (*line != '?') out[written++] = *line;
	while (written > 0 && out[written - 1] == ' ') --written;
	out[written] = '\0';
}

/* The monitor waits at a line ending in its prompt: ] in N88-BASIC, * in N-BASIC. A line that
 * closes a bracket it opened is text, as Disk BASIC's "Disk version [Oct 8,1988]". */
static int is_monitor_prompt(const char *line)
{
	const size_t length = strlen(line);
	return length > 0 && ((line[length - 1] == ']' && strchr(line, '[') == NULL) || line[length - 1] == '*');
}

/* The name of the first BASIC program on the tape in the deck: its header is ten D3h bytes and six
 * name bytes, trailing spaces dropped. A T88 image holds the bytes in its data tags (id 0101h, 12
 * bytes of timing before them); a CMT image is the bytes alone. 0 when found, else -1. */
static int tape_basic_name(char *name)
{
	static const char t88[] = "PC-8801 Tape Image(T88)";
	static unsigned char data[1 << 20];
	unsigned char header[sizeof(t88)];
	size_t count = 0, i;
	int run = 0;
	FILE *file = fopen(k88_tape_path(), "rb");
	if (file == NULL) return -1;
	if (fread(header, 1, sizeof(header), file) == sizeof(header) && memcmp(header, t88, sizeof(header)) == 0) {
		unsigned char tag[4];
		while (fread(tag, 1, 4, file) == 4) {
			const int id = tag[0] | tag[1] << 8, size = tag[2] | tag[3] << 8;
			if (id == 0) break;
			if (id == 0x0101 && size >= 12 && count + (size_t) (size - 12) <= sizeof(data)) {
				if (fseek(file, 12, SEEK_CUR) != 0 ||
				    fread(data + count, 1, (size_t) (size - 12), file) != (size_t) (size - 12)) break;
				count += (size_t) (size - 12);
			} else if (fseek(file, size, SEEK_CUR) != 0) {
				break;
			}
		}
	} else {
		rewind(file);
		count = fread(data, 1, sizeof(data), file);
	}
	fclose(file);
	for (i = 0; i < count; ++i) {
		run = data[i] == 0xd3 ? run + 1 : 0;
		if (run == 10 && i + 6 < count) {
			int length = 6;
			memcpy(name, data + i + 1, 6);
			while (length > 0 && name[length - 1] == ' ') --length;
			name[length] = '\0';
			return length > 0 ? 0 : -1;
		}
	}
	return -1;
}

int k88_loader_start(const char *script)
{
	const char *line = script;
	line_count = line_next = event_count = event_next = wait_frames = settle = 0;
	active = 0;
	if (script == NULL || *script == '\0') return 0;
	while (*line) {
		const char *end = strchr(line, '\n');
		size_t length = end ? (size_t) (end - line) : strlen(line);
		char *name;
		if (line_count == MAX_LINES || length >= MAX_LINE) return -1;
		memcpy(lines[line_count], line, length);
		lines[line_count][length] = '\0';
		/* {NAME} is the tape's first BASIC program, which N-BASIC's CLOAD must name. */
		if ((name = strstr(lines[line_count], "{NAME}")) != NULL) {
			char program[7], rest[MAX_LINE];
			if (tape_basic_name(program) != 0) {
				fprintf(stderr, "kairo88: no BASIC program on the tape %s\n", k88_tape_path());
				return -1;
			}
			snprintf(rest, sizeof(rest), "%s", name + 6);
			if ((size_t) (name - lines[line_count]) + strlen(program) + strlen(rest) >= MAX_LINE) return -1;
			strcpy(name, program);
			strcat(name, rest);
		}
		++line_count;
		line = end ? end + 1 : line + length;
	}
	active = 1;
	return 0;
}

int k88_loader_active(void)
{
	return active;
}

int k88_loader_frame(void)
{
	char last[128];
	if (!active) return 0;
	if (wait_frames > 0) {
		--wait_frames;
		return 0;
	}
	if (event_next < event_count) {
		k88_key(events[event_next].key, events[event_next].down);
		wait_frames = events[event_next].wait;
		++event_next;
		/* The last line is typed: the loader is done, and the game is the machine's. */
		if (event_next == event_count && line_next == line_count) active = 0;
		return 0;
	}
	/* Let the screen answer the last line before reading it. */
	if (settle > 0) {
		--settle;
		return 0;
	}
	last_line(last, sizeof(last));
	if (strstr(last, "How many files")) {
		if (queue_line("") != 0) return -1;
	} else if (strcmp(last, "Ok") == 0 || is_monitor_prompt(last)) {
		if (queue_line(lines[line_next++]) != 0) {
			fprintf(stderr, "kairo88: cannot type the loader line %s\n", lines[line_next - 1]);
			active = 0;
			return -1;
		}
	} else {
		return 0;
	}
	settle = SETTLE_FRAMES;
	return 0;
}
