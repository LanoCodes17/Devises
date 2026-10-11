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

/* --- Devises : valeur de 1 unite en EUR --- */
#define N_CUR 4
static const char *const currency_names[N_CUR] = {"EUR", "USD", "JPY", "CHF"};
static const float eur_value[N_CUR] = {
  1.0f,            /* 1 EUR = 1.0000 EUR */
  1.0f / 1.085f,   /* 1 USD = 0.9217 EUR */
  1.0f / 163.5f,   /* 1 JPY = 0.0061 EUR */
  1.0f / 0.945f    /* 1 CHF = 1.0582 EUR */
};

/* --- Etat de l'appli --- */
static char saisie[12];
static int saisie_len;
static int src_cur;   /* devise source (0-3) */
static int dst_cur;   /* devise cible  (0-3) */
static float resultat;

/* --- Utilitaires --- */
static void copy_str(char *dst, const char *src) {
  while (*src) *dst++ = *src++;
  *dst = 0;
}

/* --- Convertit un float en texte a 2 decimales --- */
static void fmt_float(float v, char *o) {
  if (v < 0) v = -v;
  int total = (int)(v * 100.0f + 0.5f);
  int ip = total / 100;
  int fp = total % 100;
  int n = 0;
  char t[12];
  if (ip == 0) t[n++] = '0';
  while (ip > 0) {
    t[n++] = (char)('0' + ip % 10);
    ip /= 10;
  }
  while (n > 0) *o++ = t[--n];
  *o++ = '.';
  *o++ = (char)('0' + (fp / 10));
  *o++ = (char)('0' + (fp % 10));
  *o = 0;
}

/* --- Parse un float depuis une chaine --- */
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

/* --- Recalcule le resultat --- */
static void recalc(void) {
  if (saisie_len == 0) {
    resultat = 0.0f;
    return;
  }
  float montant = parse_float(saisie);
  float in_eur = montant * eur_value[src_cur];
  resultat = in_eur / eur_value[dst_cur];
}

/* --- Affichage --- */
static void clear_line(int y) {
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, y, 320, 30}, 0xFFFF
  );
}

static void draw_source(void) {
  clear_line(60);
  eadk_display_draw_string(
    currency_names[src_cur],
    (eadk_point_t){10, 65},
    true, 0x0000, 0xFFFF
  );
  if (saisie_len == 0) {
    eadk_display_draw_string(
      "0",
      (eadk_point_t){90, 65},
      true, 0x0000, 0xFFFF
    );
    eadk_display_push_rect_uniform(
      (eadk_rect_t){100, 65, 2, 20}, 0x0000
    );
  } else {
    eadk_display_draw_string(
      saisie,
      (eadk_point_t){90, 65},
      true, 0x0000, 0xFFFF
    );
    eadk_display_push_rect_uniform(
      (eadk_rect_t){90 + saisie_len * 10, 65, 2, 20}, 0x0000
    );
  }
}

static void draw_target(void) {
  clear_line(100);
  eadk_display_draw_string(
    currency_names[dst_cur],
    (eadk_point_t){10, 105},
    true, 0x0000, 0xFFFF
  );
  char buf[24];
  fmt_float(resultat, buf);
  eadk_display_draw_string(
    buf,
    (eadk_point_t){90, 105},
    true, 0x0000, 0xFFFF
  );
}

static void draw_rate(void) {
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, 150, 320, 20}, 0xFFFF
  );
  char buf[40];
  int n = 0;
  const char *s;
  buf[n++] = '1';
  buf[n++] = ' ';
  s = currency_names[src_cur];
  while (*s) buf[n++] = *s++;
  buf[n++] = ' ';
  buf[n++] = '=';
  buf[n++] = ' ';
  float rate = eur_value[src_cur] / eur_value[dst_cur];
  char num[16];
  fmt_float(rate, num);
  s = num;
  while (*s) buf[n++] = *s++;
  buf[n++] = ' ';
  s = currency_names[dst_cur];
  while (*s) buf[n++] = *s++;
  buf[n] = 0;
  eadk_display_draw_string(
    buf,
    (eadk_point_t){10, 150},
    false, 0x0000, 0xFFFF
  );
}

static void redraw_all(void) {
  eadk_display_push_rect_uniform(
    (eadk_rect_t){0, 0, 320, 240}, 0xFFFF
  );
  eadk_display_draw_string(
    "CONVERTISSEUR",
    (eadk_point_t){10, 10},
    true, 0x0000, 0xFFFF
  );
  draw_source();
  draw_target();
  draw_rate();
}

/* --- Programme principal --- */
int main(void) {
  saisie[0] = 0;
  saisie_len = 0;
  src_cur = 0;   /* EUR */
  dst_cur = 1;   /* USD */
  resultat = 0.0f;
  redraw_all();

  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev;
    prev = k;

    if (hit == 0) {
      for (volatile int i = 0; i < 10000; i++) { }
      continue;
    }

#define HIT(key) ((hit >> (key)) & 1)

    if (HIT(KEY_HOME) || HIT(KEY_ONOFF)) return 0;

    /* devise du haut : LEFT / RIGHT */
    if (HIT(KEY_LEFT)) {
      src_cur = (src_cur + N_CUR - 1) % N_CUR;
      recalc();
      draw_source();
      draw_target();
      draw_rate();
    }
    if (HIT(KEY_RIGHT)) {
      src_cur = (src_cur + 1) % N_CUR;
      recalc();
      draw_source();
      draw_target();
      draw_rate();
    }

    /* devise du bas : UP / DOWN */
    if (HIT(KEY_UP)) {
      dst_cur = (dst_cur + N_CUR - 1) % N_CUR;
      recalc();
      draw_target();
      draw_rate();
    }
    if (HIT(KEY_DOWN)) {
      dst_cur = (dst_cur + 1) % N_CUR;
      recalc();
      draw_target();
      draw_rate();
    }

    /* chiffres */
    for (int d = 0; d < 10; d++) {
      if (HIT(digit_keys[d]) && saisie_len < 9) {
        saisie[saisie_len++] = (char)('0' + d);
        saisie[saisie_len] = 0;
        recalc();
        draw_source();
        draw_target();
      }
    }

    /* point decimal */
    if (HIT(KEY_DOT) && saisie_len < 9) {
      int has_dot = 0;
      for (int i = 0; i < saisie_len; i++) {
        if (saisie[i] == '.') { has_dot = 1; break; }
      }
      if (!has_dot) {
        if (saisie_len == 0) saisie[saisie_len++] = '0';
        saisie[saisie_len++] = '.';
        saisie[saisie_len] = 0;
        recalc();
        draw_source();
        draw_target();
      }
    }

    /* backspace */
    if (HIT(KEY_BACKSPACE)) {
      if (saisie_len > 0) {
        saisie[--saisie_len] = 0;
        recalc();
        draw_source();
        draw_target();
      }
    }
  }

  return 0;
}
