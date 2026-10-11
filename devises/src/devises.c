#include <eadk.h>

/* --- Metadonnees --- */
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Devises";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* --- Convertit un float en texte (1 chiffre apres la virgule) --- */
static void fmt1(float v, char *o) {
  /* arrondi a 1 chiffre apres la virgule */
  int total = (int)(v * 10.0f + 0.5f);   /* 108.5 -> 1085 */
  int ip = total / 10;                    /* 108 */
  int fp = total % 10;                    /* 5 */
  int n = 0;
  char t[12];
  /* ecrit les chiffres de la partie entiere a l'envers */
  if (ip == 0) t[n++] = '0';
  while (ip > 0) {
    t[n++] = (char)('0' + ip % 10);
    ip /= 10;
  }
  /* remet dans le bon ordre */
  while (n > 0) *o++ = t[--n];
  /* virgule + decimale */
  *o++ = '.';
  *o++ = (char)('0' + fp);
  *o = 0;
}

/* --- Programme principal --- */
int main(void) {
  /* fond blanc */
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, 0, 320, 240},
    0xFFFF
  );

  /* titre */
  eadk_display_draw_string(
    "CONVERTISSEUR",
    (eadk_point_t){10, 10},
    true,
    0x0000,
    0xFFFF
  );

  /* calcul : 100 EUR -> USD avec un taux de 1.085 */
  float montant = 100.0f;
  float taux = 1.085f;
  float resultat = montant * taux;

  /* affiche EUR 100.0 */
  char ligne1[24] = "EUR ";
  fmt1(montant, ligne1 + 4);
  eadk_display_draw_string(
    ligne1,
    (eadk_point_t){10, 60},
    true,
    0x0000,
    0xFFFF
  );

  /* affiche USD 108.5 */
  char ligne2[24] = "USD ";
  fmt1(resultat, ligne2 + 4);
  eadk_display_draw_string(
    ligne2,
    (eadk_point_t){10, 90},
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
