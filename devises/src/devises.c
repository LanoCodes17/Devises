#include <eadk.h>

/* --- Metadonnees --- */
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Devises";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* --- Couleurs (style NumWorks) --- */
#define RGB(r,g,b) ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define C_BAR    RGB(0xFF, 0xB6, 0x31)   /* orange de la barre */
#define C_BG     RGB(0xE7, 0xE7, 0xEF)   /* gris clair du fond */
#define C_WHITE  0xFFFF
#define C_TEXT   0x0000
#define C_GRAY   RGB(0x94, 0x9A, 0xA5)
#define C_LINE   RGB(0xC6, 0xC3, 0xC6)

/* --- Touches --- */
enum {
  KEY_LEFT = 0, KEY_UP = 1, KEY_DOWN = 2, KEY_RIGHT = 3,
  KEY_OK = 4, KEY_BACK = 5, KEY_HOME = 6, KEY_ONOFF = 8,
  KEY_BACKSPACE = 17, KEY_DOT = 49, KEY_EXE = 52
};
static const int8_t digit_keys[10] = {48, 42, 43, 44, 36, 37, 38, 30, 31, 32};

/* --- Devises --- */
#define N_CUR 4
static const char *const currency_names[N_CUR] = {"EUR", "USD", "JPY", "CHF"};
static const float eur_value[N_CUR] = {
  1.0f, 1.0f / 1.085f, 1.0f / 163.5f, 1.0f / 0.945f
};

/* --- Etat --- */
static char saisie[12];
static int saisie_len;
static int src_cur;
static int dst_cur;
static float resultat;

/* --- Largeur du texte (en pixels, approx) --- */
static int txt_w(const char *s, int size) {
  int n = 0;
  while (*s++) n++;
  return n * size;
}

/* --- Efface une zone avec la couleur du fond --- */
static void clear(int x, int y, int w, int h, uint16_t c) {
  eadk_display_push_rect_uniform((eadk_rect_t){x, y, w, h}, c);
}

/* --- Copie de chaine --- */
static void copy_str(char *d, const char *s) {
  while (*s) *d++ = *s++;
  *d = 0;
}

/* --- Float vers texte, 2 decimales --- */
static void fmt_float(float v, char *o) {
  if (v < 0) v = -v;
  int total = (int)(v * 100.0f + 0.5f);
  int ip = total / 100;
  int fp = total % 100;
  int n = 0;
  char t[12];
  if (ip == 0) t[n++] = '0';
  while (ip > 0) { t[n++] = (char)('0' + ip % 10); ip /= 10; }
  while (n > 0) *o++ = t[--n];
  *o++ = '.';
  *o++ = (char)('0' + (fp / 10));
  *o++ = (char)('0' + (fp % 10));
  *o = 0;
}

/* --- Parse float --- */
static float parse_float(const char *s) {
  float v = 0.0f, scale = 0.0f;
  for (; *s; s++) {
    if (*s == '.') scale = 1.0f;
    else if (*s >= '0' && *s <= '9') {
      if (scale > 0.0f) { scale /= 10.0f; v += (float)(*s - '0') * scale; }
      else v = v * 10.0f + (float)(*s - '0');
    }
  }
  return v;
}

/* --- Recalcul --- */
static void recalc(void) {
  if (saisie_len == 0) { resultat = 0.0f; return; }
  float m = parse_float(saisie);
  resultat = (m * eur_value[src_cur]) / eur_value[dst_cur];
}

/* --- Barre de statut (comme NumWorks) --- */
static void draw_status_bar(void) {
  clear(0, 0, 320, 18, C_BAR);
  eadk_display_draw_string("rad", (eadk_point_t){4, 2}, false, C_WHITE, C_BAR);
  /* "Devises" centre : 320/2 - largeur/2 */
  int w = txt_w("Devises", 7);
  eadk_display_draw_string("Devises", (eadk_point_t){160 - w / 2, 2}, false, C_WHITE, C_BAR);
  /* Batterie simple */
  clear(296, 5, 15, 8, C_WHITE);
  clear(297, 6, 13, 6, C_BAR);
  clear(311, 7, 2, 4, C_WHITE);
  clear(298, 7, 9, 4, C_WHITE);
}

/* --- Titre --- */
static void draw_title(void) {
  clear(0, 22, 320, 24, C_BG);
  eadk_display_draw_string("CONVERTISSEUR", (eadk_point_t){10, 26}, true, C_TEXT, C_BG);
}

/* --- Ligne devise (commune source/target) --- */
static void draw_currency_line(int y, const char *name, const char *value, int selected) {
  /* la boite blanche */
  clear(0, y, 320, 40, C_BG);
  clear(10, y, 300, 36, C_WHITE);
  /* bordure fine */
  clear(10, y, 300, 1, C_LINE);
  clear(10, y + 35, 300, 1, C_LINE);
  /* nom de devise a gauche, en gris */
  eadk_display_draw_string(name, (eadk_point_t){20, y + 10}, true, C_GRAY, C_WHITE);
  /* valeur a droite */
  int w = txt_w(value, 10);
  eadk_display_draw_string(value, (eadk_point_t){300 - w, y + 10}, true, C_TEXT, C_WHITE);
  /* petit indicateur si selectionne */
  if (selected) {
    clear(10, y, 3, 36, C_BAR);
  }
}

/* --- Taux --- */
static void draw_rate(void) {
  clear(0, 200, 320, 30, C_BG);
  /* "1 EUR = 1.08 USD" */
  char buf[40];
  int n = 0;
  const char *s;
  buf[n++] = '1'; buf[n++] = ' ';
  s = currency_names[src_cur]; while (*s) buf[n++] = *s++;
  buf[n++] = ' '; buf[n++] = '='; buf[n++] = ' ';
  float rate = eur_value[src_cur] / eur_value[dst_cur];
  char num[16];
  fmt_float(rate, num);
  s = num; while (*s) buf[n++] = *s++;
  buf[n++] = ' ';
  s = currency_names[dst_cur]; while (*s) buf[n++] = *s++;
  buf[n] = 0;
  int w = txt_w(buf, 7);
  eadk_display_draw_string(buf, (eadk_point_t){160 - w / 2, 207}, false, C_GRAY, C_BG);
}

/* --- Redessine tout --- */
static void draw_all(void) {
  clear(0, 0, 320, 240, C_BG);
  draw_status_bar();
  draw_title();

  /* Ligne source (selectionnee par defaut) */
  char buf[24];
  if (saisie_len == 0) {
    copy_str(buf, "0");
  } else {
    copy_str(buf, saisie);
  }
  draw_currency_line(60, currency_names[src_cur], buf, 1);

  /* Ligne target */
  fmt_float(resultat, buf);
  draw_currency_line(110, currency_names[dst_cur], buf, 0);

  /* Taux */
  draw_rate();
}

/* --- Mise a jour rapide (sans tout redessiner) --- */
static void refresh_values(void) {
  char buf[24];
  if (saisie_len == 0) copy_str(buf, "0");
  else copy_str(buf, saisie);
  draw_currency_line(60, currency_names[src_cur], buf, 1);

  fmt_float(resultat, buf);
  draw_currency_line(110, currency_names[dst_cur], buf, 0);

  draw_rate();
}

int main(void) {
  saisie[0] = 0; saisie_len = 0;
  src_cur = 0; dst_cur = 1;
  resultat = 0.0f;

  draw_all();

  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev;
    prev = k;
    if (hit == 0) { for (volatile int i = 0; i < 10000; i++) { } continue; }

#define HIT(key) ((hit >> (key)) & 1)

    if (HIT(KEY_HOME) || HIT(KEY_ONOFF)) return 0;

    if (HIT(KEY_LEFT))  { src_cur = (src_cur + N_CUR - 1) % N_CUR; recalc(); refresh_values(); }
    if (HIT(KEY_RIGHT)) { src_cur = (src_cur + 1) % N_CUR;         recalc(); refresh_values(); }
    if (HIT(KEY_UP))    { dst_cur = (dst_cur + N_CUR - 1) % N_CUR; recalc(); refresh_values(); }
    if (HIT(KEY_DOWN))  { dst_cur = (dst_cur + 1) % N_CUR;         recalc(); refresh_values(); }

    for (int d = 0; d < 10; d++) {
      if (HIT(digit_keys[d]) && saisie_len < 9) {
        saisie[saisie_len++] = (char)('0' + d);
        saisie[saisie_len] = 0;
        recalc(); refresh_values();
      }
    }
    if (HIT(KEY_DOT) && saisie_len < 9) {
      int has = 0;
      for (int i = 0; i < saisie_len; i++) if (saisie[i] == '.') { has = 1; break; }
      if (!has) {
        if (saisie_len == 0) saisie[saisie_len++] = '0';
        saisie[saisie_len++] = '.';
        saisie[saisie_len] = 0;
        recalc(); refresh_values();
      }
    }
    if (HIT(KEY_BACKSPACE) && saisie_len > 0) {
      saisie[--saisie_len] = 0;
      recalc(); refresh_values();
    }
  }
  return 0;
}
