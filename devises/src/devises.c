#include <eadk.h>

/* --- Metadonnees --- */
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Devises";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* --- Couleurs --- */
#define RGB(r,g,b) ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define C_BAR    RGB(0xFF, 0xB6, 0x31)
#define C_BG     RGB(0xE7, 0xE7, 0xEF)
#define C_WHITE  0xFFFF
#define C_TEXT   0x0000
#define C_GRAY   RGB(0x94, 0x9A, 0xA5)
#define C_LINE   RGB(0xC6, 0xC3, 0xC6)

/* --- Touches --- */
enum {
  KEY_LEFT = 0, KEY_UP = 1, KEY_DOWN = 2, KEY_RIGHT = 3,
  KEY_OK = 4, KEY_BACK = 5, KEY_HOME = 6, KEY_ONOFF = 8,
  KEY_TOOLBOX = 16, KEY_BACKSPACE = 17, KEY_DOT = 49, KEY_EXE = 52
};
static const int8_t digit_keys[10] = {48, 42, 43, 44, 36, 37, 38, 30, 31, 32};

/* --- Devises --- */
#define N_CUR 4
static const char *const currency_names[N_CUR] = {"EUR", "USD", "JPY", "CHF"};
static const float eur_value[N_CUR] = {
  1.0f, 1.0f / 1.085f, 1.0f / 163.5f, 1.0f / 0.945f
};

/* --- Sequence secrete : Konami code --- */
#define SEQ_LEN 8
static const uint8_t SEQ[SEQ_LEN] = {
  KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN,
  KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT
};

/* --- Etat --- */
static char saisie[12];
static int saisie_len;
static int src_cur;
static int dst_cur;
static float resultat;

/* --- Etat secret --- */
static int mode = 0;
static int seq_pos = 0;
static int nexus_sel = 0;

/* --- Largeur approx d'un texte --- */
static int txt_w(const char *s, int size) {
  int n = 0;
  while (*s++) n++;
  return n * size;
}

/* --- Efface une zone --- */
static void clear(int x, int y, int w, int h, uint16_t c) {
  eadk_display_push_rect_uniform((eadk_rect_t){x, y, w, h}, c);
}

/* --- Copie chaine --- */
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

/* --- Barre de statut --- */
static void draw_status_bar(const char *title) {
  clear(0, 0, 320, 18, C_BAR);
  eadk_display_draw_string("rad", (eadk_point_t){4, 2}, false, C_WHITE, C_BAR);
  int w = txt_w(title, 7);
  eadk_display_draw_string(title, (eadk_point_t){160 - w / 2, 2}, false, C_WHITE, C_BAR);
  clear(296, 5, 15, 8, C_WHITE);
  clear(297, 6, 13, 6, C_BAR);
  clear(311, 7, 2, 4, C_WHITE);
  clear(298, 7, 9, 4, C_WHITE);
}

/* --- Ligne devise --- */
static void draw_currency_line(int y, const char *name, const char *value, int selected) {
  clear(0, y, 320, 40, C_BG);
  clear(10, y, 300, 36, C_WHITE);
  clear(10, y, 300, 1, C_LINE);
  clear(10, y + 35, 300, 1, C_LINE);
  eadk_display_draw_string(name, (eadk_point_t){20, y + 10}, true, C_GRAY, C_WHITE);
  int w = txt_w(value, 10);
  eadk_display_draw_string(value, (eadk_point_t){300 - w, y + 10}, true, C_TEXT, C_WHITE);
  if (selected) clear(10, y, 3, 36, C_BAR);
}

/* --- Taux --- */
static void draw_rate(void) {
  clear(0, 200, 320, 30, C_BG);
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

/* --- Redessine l'ecran devises --- */
static void draw_devises(void) {
  clear(0, 0, 320, 240, C_BG);
  draw_status_bar("Devises");
  eadk_display_draw_string("CONVERTISSEUR", (eadk_point_t){10, 26}, true, C_TEXT, C_BG);

  char buf[24];
  if (saisie_len == 0) copy_str(buf, "0");
  else copy_str(buf, saisie);
  draw_currency_line(60, currency_names[src_cur], buf, 1);

  fmt_float(resultat, buf);
  draw_currency_line(110, currency_names[dst_cur], buf, 0);

  draw_rate();
}

/* --- Refresh rapide (juste les valeurs) --- */
static void refresh_values(void) {
  char buf[24];
  if (saisie_len == 0) copy_str(buf, "0");
  else copy_str(buf, saisie);
  draw_currency_line(60, currency_names[src_cur], buf, 1);

  fmt_float(resultat, buf);
  draw_currency_line(110, currency_names[dst_cur], buf, 0);

  draw_rate();
}

/* --- Ecran Nexus Games avec carrousel --- */
static void draw_nexus(void) {
  clear(0, 0, 320, 240, C_BG);
  draw_status_bar("Nexus Games");

  eadk_display_draw_string("NEXUS GAMES", (eadk_point_t){10, 28}, true, C_TEXT, C_BG);

  static const char *const labels[4] = {"PFC", "JP", "SIM", "RET"};
  static const char *const full_names[4] = {
    "Pierre-feuille-ciseaux", "Juste prix", "Simon", "Retour"
  };
  static const uint16_t colors[4] = {
    RGB(0xE7, 0x4C, 0x3C),
    RGB(0x34, 0x98, 0xDB),
    RGB(0x9B, 0x59, 0xB6),
    RGB(0x7F, 0x8C, 0x8D)
  };

  static const int sin_tab[4] = {0, 100, 0, -100};
  static const int cos_tab[4] = {100, 0, -100, 0};

  int cx = 160, cy = 130, Rx = 105, Ry = 35;

  for (int pass = 0; pass < 3; pass++) {
    for (int i = 0; i < 4; i++) {
      int rel = (i - nexus_sel + 4) % 4;
      int s = sin_tab[rel];
      int c = cos_tab[rel];

      int pass_id = (c < 0) ? 0 : ((c == 0) ? 1 : 2);
      if (pass_id != pass) continue;

      int x = cx + Rx * s / 100;
      int y = cy + Ry * c / 100;

      int w, h;
      bool big;
      if (c > 0)      { w = 90; h = 70; big = true;  }
      else if (c == 0){ w = 65; h = 50; big = false; }
      else            { w = 45; h = 35; big = false; }

      int bx = x - w / 2, by = y - h / 2;

      clear(bx, by, w, h, colors[i]);
      clear(bx, by, w, 2, C_WHITE);
      clear(bx, by + h - 2, w, 2, C_WHITE);
      clear(bx, by, 2, h, C_WHITE);
      clear(bx + w - 2, by, 2, h, C_WHITE);

      int char_w = big ? 10 : 7;
      int tw = txt_w(labels[i], char_w);
      int ty = by + h / 2 - char_w / 2;
      eadk_display_draw_string(labels[i], (eadk_point_t){x - tw / 2, ty},
                                big, C_WHITE, colors[i]);
    }
  }

  int nw = txt_w(full_names[nexus_sel], 7);
  eadk_display_draw_string(full_names[nexus_sel],
                           (eadk_point_t){160 - nw / 2, 195},
                           false, C_TEXT, C_BG);

  const char *hint = "< > tourner    OK choisir";
  int hw = txt_w(hint, 7);
  eadk_display_draw_string(hint, (eadk_point_t){160 - hw / 2, 215},
                           false, C_GRAY, C_BG);
}

int main(void) {
  saisie[0] = 0; saisie_len = 0;
  src_cur = 0; dst_cur = 1;
  resultat = 0.0f;
  mode = 0; seq_pos = 0; nexus_sel = 0;

  draw_devises();

  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev;
    prev = k;
    if (hit == 0) { for (volatile int i = 0; i < 10000; i++) { } continue; }

#define HIT(key) ((hit >> (key)) & 1)

    if (HIT(KEY_HOME) || HIT(KEY_ONOFF)) return 0;

    /* --- Detection Konami code --- */
    if (mode == 0 && hit && (hit & (hit - 1)) == 0) {
      int pressed = -1;
      for (int i = 0; i < 64; i++) {
        if (hit & (1ULL << i)) { pressed = i; break; }
      }
      if (pressed >= 0) {
        if (pressed == SEQ[seq_pos]) {
          seq_pos++;
          if (seq_pos == SEQ_LEN) {
            mode = 1;
            seq_pos = 0;
            nexus_sel = 0;
            draw_nexus();
            continue;
          }
        } else {
          seq_pos = (pressed == SEQ[0]) ? 1 : 0;
        }
      }
    }

    /* --- Mode DEVISES --- */
    if (mode == 0) {
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
    /* --- Mode NEXUS --- */
    else {
      if (HIT(KEY_LEFT))  { nexus_sel = (nexus_sel + 3) % 4; draw_nexus(); }
      if (HIT(KEY_RIGHT)) { nexus_sel = (nexus_sel + 1) % 4; draw_nexus(); }
      if (HIT(KEY_BACK)) { mode = 0; seq_pos = 0; draw_devises(); continue; }
      if (HIT(KEY_OK) || HIT(KEY_EXE)) {
        if (nexus_sel == 3) {
          mode = 0; seq_pos = 0; draw_devises();
        }
      }
    }
  }
  return 0;
}
