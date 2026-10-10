#include <eadk.h>

/* --- Metadonnees (l'icone vient du PNG, pas du code) --- */
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Devises";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* --- Programme principal --- */
int main(void) {
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, 0, 320, 240},
    0xFFFF
  );

  eadk_display_draw_string(
    "DEVISES",
    (eadk_point_t){10, 10},
    true,
    0x0000,
    0xFFFF
  );

  while (1) {
    for (volatile int i = 0; i < 500000; i++) {
    }
  }

  return 0;
}
