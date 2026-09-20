/* JANUS: compiler probe only, not a world/capability test. See LICENSING.md. */
#include <cstddef>
#include <memory>
#include <new>

int
main()
{
	std::unique_ptr<unsigned char[]> buffer(
	    new (std::nothrow) unsigned char[32]());

	if (!buffer)
		return (1);
	for (std::size_t i = 0; i < 32; i++)
		buffer[i] = static_cast<unsigned char>(i);
	if (buffer[31] != 31)
		return (1);
	return (0);
}
