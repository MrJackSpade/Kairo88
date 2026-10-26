/* What the PC-88 needs for a game's startup commands, which the shared typer types
 * (shared/frontend/src/main/cpp/kairo/startup_typer.h, through kairo88_typing.cpp): the prompt the
 * screen shows, the key that types a character, and a tape's program name. */
#include <stdio.h>
#include <string.h>

#include "kairo88_core.h"

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

int k88_prompt(void)
{
	char last[128];
	last_line(last, sizeof(last));
	if (strstr(last, "How many files")) return K88_PROMPT_RETURN;
	if (strcmp(last, "Ok") == 0 || is_monitor_prompt(last)) return K88_PROMPT_LINE;
	return K88_PROMPT_NONE;
}

int k88_typed_key(char c, int *shifted)
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

int k88_expand_name(char *line, int size)
{
	char *name = strstr(line, "{NAME}");
	char program[7], rest[256];
	if (name == NULL) return 0;
	if (tape_basic_name(program) != 0) {
		fprintf(stderr, "kairo88: no BASIC program on the tape %s\n", k88_tape_path());
		return -1;
	}
	snprintf(rest, sizeof(rest), "%s", name + 6);
	if ((size_t) (name - line) + strlen(program) + strlen(rest) >= (size_t) size) return -1;
	strcpy(name, program);
	strcat(name, rest);
	return 0;
}
