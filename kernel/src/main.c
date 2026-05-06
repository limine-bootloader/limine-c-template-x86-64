#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

// Convert HSV color values to RGB

[[gnu::always_inline]] inline uint32_t hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v)
{
	uint8_t region = h / 60;
	uint16_t remainder = (h - (region * 60)) * 255 / 60;

	uint8_t p = (v * (255 - s)) / 255;
	uint8_t q = (v * (255 - (s * remainder) / 255)) / 255;
	uint8_t t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;

	switch (region)
	{
	case 0:
		return (v << 16) | (t << 8) | p;
	case 1:
		return (q << 16) | (v << 8) | p;
	case 2:
		return (p << 16) | (v << 8) | t;
	case 3:
		return (p << 16) | (q << 8) | v;
	case 4:
		return (t << 16) | (p << 8) | v;
	default:
		return (v << 16) | (p << 8) | q;
	}
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
[[gnu::noreturn]] void kmain(void)
{
	// Ensure the bootloader actually understands our base revision (see spec).
	if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false)
	{
		hcf();
	}

	// Ensure we got a framebuffer.
	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
	{
		hcf();
	}

	// Fetch the first framebuffer.
	struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
	// Print a nice pattern to screen as an example.
	// Note: we assume the framebuffer model is RGB with 32-bit pixels.
	uint32_t *fb_ptr = framebuffer->address;
	uint32_t pitch = framebuffer->pitch / 4;
	uint64_t offset = 0;
	do
	{
		for (uint32_t y = 0; y < framebuffer->height; y++)
		{
			uint32_t *row = fb_ptr + y * pitch;
			for (uint32_t x = 0; x < framebuffer->width; x++)
			{
				uint32_t v1 = x ^ y;
				uint32_t v2 = (x + offset) ^ (y + offset);
				uint32_t v = (v1 + v2 + (v1 >> 1) + (v2 >> 1)) / 3 + offset;
				uint16_t hue = (v * 3) % 360;
				row[x] = hsv_to_rgb(hue, 200, offset < 255 ? offset : 255);
			}
		}
	} while (offset++ || true);
}