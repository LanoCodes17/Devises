
#include <eadk.h>

/* --- Metadonnees de l'appli (obligatoires pour NumWorks) --- */
const char eadk_app_name[] = "Devises";
const int eadk_api_level = 0;
const uint8_t eadk_app_icon[] = {
  /* icone minimale 1x1 (remplacee plus tard) */
  0x00
};

/* --- Programme principal --- */
__attribute__((used, section(".text.main")))
int main(void) {
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, 0, 320, 240},
    (eadk_color_t){0xFF, 0xFF, 0xFF}
  );

  eadk_display_draw_string(
    "DEVISES",
    (eadk_point_t){10, 10},
    true,
    (eadk_color_t){0x00, 0x00, 0x00},
    (eadk_color_t){0xFF, 0xFF, 0xFF}
  );

  while (1) {
    eadk_sleep(100);
  }

  return 0;
}
