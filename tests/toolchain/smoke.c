/* JANUS: compiler probe only, not a world/capability test. See LICENSING.md. */
#include <stddef.h>
#include <stdlib.h>

int
main(void)
{
	unsigned char *buffer;
	size_t i;

	buffer = calloc(32, sizeof(*buffer));
	if (buffer == NULL)
		return (1);
	for (i = 0; i < 32; i++)
		buffer[i] = (unsigned char)i;
	if (buffer[31] != 31) {
		free(buffer);
		return (1);
	}
	free(buffer);
	return (0);
}
