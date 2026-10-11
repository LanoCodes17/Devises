#include <eadk.h>

/* --- Metadonnees --- */
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Devises";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* --- Touches (eadk numbering) --- */
enum {
  KEY_LEFT = 0, KEY_UP = 1, KEY_DOWN = 2, KEY_RIGHT = 3,
  KEY_OK = 4, KEY_BACK = 5, KEY_HOME = 6, KEY_ONOFF = 8,
  KEY_BACKSPACE = 17, KEY_DOT = 49, KEY_EXE = 52
};
static const int8_t digit_keys[10] = {48, 42, 43, 44, 36, 37, 38, 30, 31, 32};

/* --- Etat de l'appli --- */
static char saisie[12];       /* texte tape par l'utilisateur */
static int saisie_len;        /* longueur actuelle */
static float resultat;        /* resultat du calcul */

/* --- Copie de chaine (sans memset) --- */
static void copy_str(char *dst, const char *src) {
  while (*src) *dst++ = *src++;
  *dst = 0;
}

/* --- Convertit un float en texte (1 chiffre apres la virgule) --- */
static void fmt1(float v, char *o) {
  int total = (int)(v * 10.0f + 0.5f);
  int ip = total / 10;
  int fp = total % 10;
  int n = 0;
  char t[12];
  if (ip == 0) t[n++] = '0';
  while (ip > 0) {
    t[n++] = (char)('0' + ip % 10);
    ip /= 10;
  }
  while (n > 0) *o++ = t[--n];
  *o++ = '.';
  *o++ = (char)('0' + fp);
  *o = 0;
}

/* --- Parse un float depuis une chaine (sans atof) --- */
static float parse_float(const char *s) {
  float v = 0.0f;
  float scale = 0.0f;
  for (; *s; s++) {
    if (*s == '.') {
      scale = 1.0f;
    } else if (*s >= '0' && *s <= '9') {
      if (scale > 0.0f) {
        scale /= 10.0f;
        v += (float)(*s - '0') * scale;
      } else {
        v = v * 10.0f + (float)(*s - '0');
      }
    }
  }
  return v;
}

/* --- Efface une zone --- */
static void clear_zone(int x, int y, int w, int h) {
  eadk_display_push_rect_uniform(
    (eadk_rect_t){x, y, w, h},
    0xFFFF
  );
}

/* --- Redessine la ligne EUR (avec le curseur) --- */
static void draw_eur(void) {
  clear_zone(0, 60, 320, 25);
  eadk_display_draw_string(
    "EUR",
    (eadk_point_t){10, 60},
    true, 0x0000, 0xFFFF
  );
  if (saisie_len == 0) {
    /* rien tape : affiche 0 */
    eadk_display_draw_string(
      "0_",
      (eadk_point_t){70, 60},
      true, 0x0000, 0xFFFF
    );
  } else {
    eadk_display_draw_string(
      saisie,
      (eadk_point_t){70, 60},
      true, 0x0000, 0xFFFF
    );
    /* curseur apres la saisie */
    eadk_display_draw_string(
      "_",
      (eadk_point_t){70 + saisie_len * 10, 60},
      true, 0x0000, 0xFFFF
    );
  }
}

/* --- Redessine la ligne USD (resultat) --- */
static void draw_usd(void) {
  clear_zone(0, 90, 320, 25);
  eadk_display_draw_string(
    "USD",
    (eadk_point_t){10, 90},
    true, 0x0000, 0xFFFF
  );
  char buf[24];
  fmt1(resultat, buf);
  eadk_display_draw_string(
    buf,
    (eadk_point_t){70, 90},
    true, 0x0000, 0xFFFF
  );
}

/* --- Recalcule le resultat --- */
static void recalc(void) {
  if (saisie_len == 0) {
    resultat = 0.0f;
  } else {
    float montant = parse_float(saisie);
    resultat = montant * 1.085f;
  }
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
    true, 0x0000, 0xFFFF
  );

  /* init */
  saisie[0] = 0;
  saisie_len = 0;
  resultat = 0.0f;
  draw_eur();
  draw_usd();

  /* boucle principale */
  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev;
    prev = k;

    if (hit == 0) {
      /* pas de touche : attendre un peu */
      for (volatile int i = 0; i < 10000; i++) { }
      continue;
    }

#define HIT(key) ((hit >> (key)) & 1)

    /* quitter */
    if (HIT(KEY_HOME) || HIT(KEY_ONOFF)) {
      return 0;
    }

    /* chiffres */
    for (int d = 0; d < 10; d++) {
      if (HIT(digit_keys[d]) && saisie_len < 9) {
        saisie[saisie_len++] = (char)('0' + d);
        saisie[saisie_len] = 0;
        recalc();
        draw_eur();
        draw_usd();
      }
    }

    /* point decimal */
    if (HIT(KEY_DOT) && saisie_len < 9) {
      /* verifie qu'il n'y a pas deja un point */
      int has_dot = 0;
      for (int i = 0; i < saisie_len; i++) {
        if (saisie[i] == '.') { has_dot = 1; break; }
      }
      if (!has_dot) {
        /* si vide, met "0." ; sinon ajoute "." */
        if (saisie_len == 0) {
          saisie[saisie_len++] = '0';
        }
        saisie[saisie_len++] = '.';
        saisie[saisie_len] = 0;
        recalc();
        draw_eur();
        draw_usd();
      }
    }

    /* backspace */
    if (HIT(KEY_BACKSPACE)) {
      if (saisie_len > 0) {
        saisie[--saisie_len] = 0;
        recalc();
        draw_eur();
        draw_usd();
      }
    }
  }

  return 0;
}
