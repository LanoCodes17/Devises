
#include <eadk.h>

/* --- Metadonnees de l'appli (obligatoires pour NumWorks) --- */
const char eadk_app_name[] = "Devises";
const int eadk_api_level = 0;
const uint8_t eadk_app_icon[] = {
  /* icone minimale vide (on mettra une vraie plus tard) */
  0x00
};

/* --- Programme principal --- */
int main(void) {
  /* Efface l'ecran en blanc (couleur 0xFFFF en RGB565) */
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, 0, 320, 240},
    0xFFFF
  );

  /* Affiche "DEVISES" en haut a gauche, en noir sur blanc */
  eadk_display_draw_string(
    "DEVISES",
    (eadk_point_t){10, 10},
    true,
    0x0000,
    0xFFFF
  );

  /* Boucle infinie */
  while (1) {
    /* attend environ 100 ms (100000 microsecondes) */
    eadk_delay_microseconds(100000);
  }

  return 0;
}
