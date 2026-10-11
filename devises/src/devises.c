
#include <eadk.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Devises";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define RGB(r,g,b) ((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3)))
#define C_BAR    RGB(0xFF, 0xB6, 0x31)
#define C_BG     RGB(0xE7, 0xE7, 0xEF)
#define C_WHITE  0xFFFF
#define C_TEXT   0x0000
#define C_GRAY   RGB(0x94, 0x9A, 0xA5)
#define C_LINE   RGB(0xC6, 0xC3, 0xC6)
#define C_RED    RGB(0xE7, 0x4C, 0x3C)
#define C_GREEN  RGB(0x27, 0xAE, 0x60)
#define C_BLUE   RGB(0x34, 0x98, 0xDB)
#define C_PURPLE RGB(0x9B, 0x59, 0xB6)
#define C_YELLOW RGB(0xF1, 0xC4, 0x0F)

enum {
  KEY_LEFT = 0, KEY_UP = 1, KEY_DOWN = 2, KEY_RIGHT = 3,
  KEY_OK = 4, KEY_BACK = 5, KEY_HOME = 6, KEY_ONOFF = 8,
  KEY_TOOLBOX = 16, KEY_BACKSPACE = 17, KEY_DOT = 49, KEY_EXE = 52
};
static const int8_t digit_keys[10] = {48, 42, 43, 44, 36, 37, 38, 30, 31, 32};

#define N_CUR 4
static const char *const currency_names[N_CUR] = {"EUR", "USD", "JPY", "CHF"};
static const float eur_value[N_CUR] = {1.0f, 1.0f/1.085f, 1.0f/163.5f, 1.0f/0.945f};

#define SEQ_LEN 8
static const uint8_t SEQ[SEQ_LEN] = {
  KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT
};

static uint32_t rng_state = 1;
static uint32_t rng_next(void) {
  rng_state = rng_state * 1103515245u + 12345u;
  return (rng_state >> 16) & 0x7FFF;
}

static char saisie[12];
static int saisie_len;
static int src_cur, dst_cur;
static float resultat;
static int mode = 0;
static int seq_pos = 0;
static int nexus_sel = 0;

static int txt_w(const char *s, int size) { int n=0; while (*s++) n++; return n*size; }
static void clear(int x, int y, int w, int h, uint16_t c) {
  eadk_display_push_rect_uniform((eadk_rect_t){x,y,w,h}, c);
}
static void copy_str(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; }
static void fmt_float(float v, char *o) {
  if (v<0) v=-v;
  int total=(int)(v*100.0f+0.5f), ip=total/100, fp=total%100, n=0;
  char t[12];
  if (!ip) t[n++]='0';
  while (ip>0) { t[n++]=(char)('0'+ip%10); ip/=10; }
  while (n>0) *o++=t[--n];
  *o++='.'; *o++=(char)('0'+fp/10); *o++=(char)('0'+fp%10); *o=0;
}
static float parse_float(const char *s) {
  float v=0.0f, sc=0.0f;
  for (; *s; s++) {
    if (*s=='.') sc=1.0f;
    else if (*s>='0'&&*s<='9') {
      if (sc>0) { sc/=10.0f; v+=(float)(*s-'0')*sc; } else v=v*10.0f+(float)(*s-'0');
    }
  }
  return v;
}
static void recalc(void) {
  if (!saisie_len) { resultat=0.0f; return; }
  resultat = (parse_float(saisie)*eur_value[src_cur])/eur_value[dst_cur];
}
static void int_to_str(int v, char *o) {
  int n=0; char t[8];
  if (!v) t[n++]='0';
  while (v>0) { t[n++]=(char)('0'+v%10); v/=10; }
  while (n>0) *o++=t[--n];
  *o=0;
}

static void draw_status_bar(const char *title) {
  clear(0,0,320,18,C_BAR);
  eadk_display_draw_string("rad",(eadk_point_t){4,2},false,C_WHITE,C_BAR);
  int w = txt_w(title,7);
  eadk_display_draw_string(title,(eadk_point_t){160-w/2,2},false,C_WHITE,C_BAR);
  clear(296,5,15,8,C_WHITE); clear(297,6,13,6,C_BAR); clear(311,7,2,4,C_WHITE); clear(298,7,9,4,C_WHITE);
}

/* ============ DEVISES ============ */
static void draw_currency_line(int y, const char *name, const char *value, int sel) {
  clear(0,y,320,40,C_BG);
  clear(10,y,300,36,C_WHITE);
  clear(10,y,300,1,C_LINE); clear(10,y+35,300,1,C_LINE);
  eadk_display_draw_string(name,(eadk_point_t){20,y+10},true,C_GRAY,C_WHITE);
  int w=txt_w(value,10);
  eadk_display_draw_string(value,(eadk_point_t){300-w,y+10},true,C_TEXT,C_WHITE);
  if (sel) clear(10,y,3,36,C_BAR);
}
static void draw_rate(void) {
  clear(0,200,320,30,C_BG);
  char buf[40]; int n=0;
  const char *s;
  buf[n++]='1'; buf[n++]=' ';
  s=currency_names[src_cur]; while (*s) buf[n++]=*s++;
  buf[n++]=' '; buf[n++]='='; buf[n++]=' ';
  float rate = eur_value[src_cur]/eur_value[dst_cur];
  char num[16]; fmt_float(rate,num);
  s=num; while (*s) buf[n++]=*s++;
  buf[n++]=' ';
  s=currency_names[dst_cur]; while (*s) buf[n++]=*s++;
  buf[n]=0;
  int w=txt_w(buf,7);
  eadk_display_draw_string(buf,(eadk_point_t){160-w/2,207},false,C_GRAY,C_BG);
}
static void draw_devises(void) {
  clear(0,0,320,240,C_BG);
  draw_status_bar("Devises");
  eadk_display_draw_string("CONVERTISSEUR",(eadk_point_t){10,26},true,C_TEXT,C_BG);
  char buf[24];
  if (!saisie_len) copy_str(buf,"0"); else copy_str(buf,saisie);
  draw_currency_line(60,currency_names[src_cur],buf,1);
  fmt_float(resultat,buf);
  draw_currency_line(110,currency_names[dst_cur],buf,0);
  draw_rate();
}
static void refresh_values(void) {
  char buf[24];
  if (!saisie_len) copy_str(buf,"0"); else copy_str(buf,saisie);
  draw_currency_line(60,currency_names[src_cur],buf,1);
  fmt_float(resultat,buf);
  draw_currency_line(110,currency_names[dst_cur],buf,0);
  draw_rate();
}

/* ============ NEXUS CARROUSEL ============ */
static void draw_nexus(void) {
  clear(0,0,320,240,C_BG);
  draw_status_bar("Nexus Games");
  eadk_display_draw_string("NEXUS GAMES",(eadk_point_t){10,28},true,C_TEXT,C_BG);
  static const char *const labels[4]={"PFC","JP","SIM","RET"};
  static const char *const full_names[4]={"Pierre-feuille-ciseaux","Juste prix","Simon","Retour"};
  static const uint16_t colors[4]={C_RED,C_BLUE,C_PURPLE,RGB(0x7F,0x8C,0x8D)};
  static const int sin_tab[4]={0,100,0,-100};
  static const int cos_tab[4]={100,0,-100,0};
  int cx=160, cy=130, Rx=105, Ry=35;
  for (int pass=0; pass<3; pass++) {
    for (int i=0;i<4;i++) {
      int rel=(i-nexus_sel+4)%4;
      int s=sin_tab[rel], c=cos_tab[rel];
      int pass_id=(c<0)?0:((c==0)?1:2);
      if (pass_id != pass) continue;
      int x=cx+Rx*s/100, y=cy+Ry*c/100;
      int w,h; bool big;
      if (c>0) { w=90; h=70; big=true; }
      else if (c==0) { w=65; h=50; big=false; }
      else { w=45; h=35; big=false; }
      int bx=x-w/2, by=y-h/2;
      clear(bx,by,w,h,colors[i]);
      clear(bx,by,w,2,C_WHITE); clear(bx,by+h-2,w,2,C_WHITE);
      clear(bx,by,2,h,C_WHITE); clear(bx+w-2,by,2,h,C_WHITE);
      int chw = big?10:7;
      int tw = txt_w(labels[i],chw);
      int ty = by+h/2-chw/2;
      eadk_display_draw_string(labels[i],(eadk_point_t){x-tw/2,ty},big,C_WHITE,colors[i]);
    }
  }
  int nw=txt_w(full_names[nexus_sel],7);
  eadk_display_draw_string(full_names[nexus_sel],(eadk_point_t){160-nw/2,195},false,C_TEXT,C_BG);
  const char *hint="< > tourner    OK choisir";
  int hw=txt_w(hint,7);
  eadk_display_draw_string(hint,(eadk_point_t){160-hw/2,215},false,C_GRAY,C_BG);
}

/* ============ PFC ============ */
#define PFC_PIERRE 0
#define PFC_FEUILLE 1
#define PFC_CISEAUX 2
static const char *const pfc_names[3]={"PIERRE","FEUILLE","CISEAUX"};
static int pfc_player=0, pfc_bot=0, pfc_phase=0, pfc_anim=0;
static int pfc_score_p=0, pfc_score_b=0, pfc_result=-1;

static void pfc_draw_hand(int cx, int cy, int size, int choice, uint16_t color, int alpha) {
  uint16_t col = alpha ? C_GRAY : color;
  int half = size/2;
  if (choice == PFC_PIERRE) {
    clear(cx-half, cy-half, size, size, col);
  } else if (choice == PFC_FEUILLE) {
    clear(cx-half, cy-half+size/6, size, size-size/3, col);
  } else {
    clear(cx-half, cy-3, size, 6, col);
    clear(cx-3, cy-half, 6, size, col);
    for (int i=0; i<size/2; i++) {
      clear(cx-half+i*2, cy-half+i, 3, 3, col);
      clear(cx+half-i*2, cy-half+i, 3, 3, col);
    }
  }
}

static void pfc_draw(void) {
  clear(0,0,320,240,C_BG);
  draw_status_bar("PFC");
  char sbuf[20]; int sn=0;
  sbuf[sn++]='T'; sbuf[sn++]=':'; sbuf[sn++]=' ';
  sbuf[sn++]=(char)('0'+pfc_score_p);
  sbuf[sn++]=' '; sbuf[sn++]='O'; sbuf[sn++]=':'; sbuf[sn++]=' ';
  sbuf[sn++]=(char)('0'+pfc_score_b); sbuf[sn]=0;
  eadk_display_draw_string(sbuf,(eadk_point_t){10,22},false,C_TEXT,C_BG);
  const char *t="PIERRE - FEUILLE - CISEAUX";
  int tw=txt_w(t,7);
  eadk_display_draw_string(t,(eadk_point_t){160-tw/2,45},false,C_GRAY,C_BG);
  int left_cx=80, right_cx=240, hand_cy=110;
  int y_off=0;
  if (pfc_phase==1) { int step=(pfc_anim/10)%2; y_off = step?12:-12; }
  pfc_draw_hand(left_cx, hand_cy+y_off, 60, pfc_player, C_BLUE, 0);
  if (pfc_phase==1 && pfc_anim<60) pfc_draw_hand(right_cx, hand_cy+y_off, 60, PFC_PIERRE, C_RED, 1);
  else pfc_draw_hand(right_cx, hand_cy+y_off, 60, pfc_bot, C_RED, 0);
  int n1w=txt_w(pfc_names[pfc_player],7);
  eadk_display_draw_string(pfc_names[pfc_player],(eadk_point_t){left_cx-n1w/2,165},false,C_TEXT,C_BG);
  if (pfc_phase==2) {
    int n2w=txt_w(pfc_names[pfc_bot],7);
    eadk_display_draw_string(pfc_names[pfc_bot],(eadk_point_t){right_cx-n2w/2,165},false,C_TEXT,C_BG);
  }
  if (pfc_phase==0) {
    const char *h="< > choisir    OK jouer";
    int hw=txt_w(h,7);
    eadk_display_draw_string(h,(eadk_point_t){160-hw/2,200},false,C_GRAY,C_BG);
    int cw=txt_w(pfc_names[pfc_player],10);
    eadk_display_draw_string(pfc_names[pfc_player],(eadk_point_t){160-cw/2,185},true,C_TEXT,C_BG);
  } else if (pfc_phase==1) {
    const char *h="PIERRE... FEUILLE... CISEAUX...";
    int hw=txt_w(h,7);
    eadk_display_draw_string(h,(eadk_point_t){160-hw/2,200},false,C_GRAY,C_BG);
  } else {
    const char *r; uint16_t col;
    if (pfc_result==0) { r="EGALITE"; col=C_GRAY; }
    else if (pfc_result==1) { r="GAGNE !"; col=C_GREEN; }
    else { r="PERDU"; col=C_RED; }
    int rw=txt_w(r,10);
    eadk_display_draw_string(r,(eadk_point_t){160-rw/2,190},true,col,C_BG);
    const char *h="OK rejouer    BACK sortir";
    int hw=txt_w(h,7);
    eadk_display_draw_string(h,(eadk_point_t){160-hw/2,215},false,C_GRAY,C_BG);
  }
}

static int pfc_play(void) {
  pfc_player=0; pfc_bot=0; pfc_phase=0; pfc_anim=0; pfc_result=-1;
  pfc_draw();
  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev; prev = k;
#define PHIT(key) ((hit >> (key)) & 1)
    if (PHIT(KEY_HOME) || PHIT(KEY_ONOFF)) return 0;
    if (pfc_phase==0) {
      if (PHIT(KEY_LEFT)) { pfc_player=(pfc_player+2)%3; pfc_draw(); }
      if (PHIT(KEY_RIGHT)) { pfc_player=(pfc_player+1)%3; pfc_draw(); }
      if (PHIT(KEY_BACK)) return 1;
      if (PHIT(KEY_OK)||PHIT(KEY_EXE)) {
        pfc_bot=rng_next()%3; pfc_phase=1; pfc_anim=0; pfc_draw();
      }
    } else if (pfc_phase==1) {
      pfc_anim++;
      pfc_draw();
      if (pfc_anim>=75) {
        if (pfc_player==pfc_bot) pfc_result=0;
        else if ((pfc_player+1)%3==pfc_bot) pfc_result=2;
        else pfc_result=1;
        if (pfc_result==1) pfc_score_p++;
        else if (pfc_result==2) pfc_score_b++;
        pfc_phase=2; pfc_draw();
      }
    } else {
      if (PHIT(KEY_BACK)) return 1;
      if (PHIT(KEY_OK)||PHIT(KEY_EXE)) {
        pfc_phase=0; pfc_anim=0; pfc_result=-1; pfc_draw();
      }
    }
    for (volatile int i=0;i<5000;i++) { }
  }
}

/* ============ JUSTE PRIX ============ */
static int jp_secret=0, jp_tries=0;
static char jp_input[8];
static int jp_input_len=0, jp_phase=0, jp_hint=0;

static void jp_draw(void) {
  clear(0,0,320,240,C_BG);
  draw_status_bar("Juste Prix");
  const char *t="JUSTE PRIX";
  int tw=txt_w(t,10);
  eadk_display_draw_string(t,(eadk_point_t){160-tw/2,30},true,C_TEXT,C_BG);
  const char *st="Devine entre 1 et 1000";
  int stw=txt_w(st,7);
  eadk_display_draw_string(st,(eadk_point_t){160-stw/2,58},false,C_GRAY,C_BG);
  char sbuf[24]; int sn=0;
  const char *pref="Essais : ";
  while (*pref) sbuf[sn++]=*pref++;
  sbuf[sn++]=(char)('0'+(jp_tries/10));
  sbuf[sn++]=(char)('0'+(jp_tries%10));
  sbuf[sn++]=' '; sbuf[sn++]='/'; sbuf[sn++]=' ';
  sbuf[sn++]='1'; sbuf[sn++]='0'; sbuf[sn]=0;
  int sw=txt_w(sbuf,7);
  eadk_display_draw_string(sbuf,(eadk_point_t){160-sw/2,85},false,C_TEXT,C_BG);
  clear(80,115,160,42,C_WHITE);
  clear(80,115,160,1,C_LINE); clear(80,156,160,1,C_LINE);
  if (!jp_input_len) eadk_display_draw_string("_",(eadk_point_t){155,128},true,C_GRAY,C_WHITE);
  else {
    int iw=txt_w(jp_input,10);
    eadk_display_draw_string(jp_input,(eadk_point_t){160-iw/2,128},true,C_TEXT,C_WHITE);
  }
  if (jp_phase==1) {
    const char *msg; uint16_t col;
    if (jp_hint==1) { msg="PLUS GRAND !"; col=C_BLUE; }
    else { msg="PLUS PETIT !"; col=C_RED; }
    int mw=txt_w(msg,10);
    eadk_display_draw_string(msg,(eadk_point_t){160-mw/2,172},true,col,C_BG);
  } else if (jp_phase==2) {
    const char *msg="GAGNE !";
    int mw=txt_w(msg,10);
    eadk_display_draw_string(msg,(eadk_point_t){160-mw/2,172},true,C_GREEN,C_BG);
    char ebuf[24]; int en=0;
    const char *ep="en ";
    while (*ep) ebuf[en++]=*ep++;
    ebuf[en++]=(char)('0'+(jp_tries/10));
    ebuf[en++]=(char)('0'+(jp_tries%10));
    const char *suf=" essais"; while (*suf) ebuf[en++]=*suf++;
    ebuf[en]=0;
    int ew=txt_w(ebuf,7);
    eadk_display_draw_string(ebuf,(eadk_point_t){160-ew/2,195},false,C_GRAY,C_BG);
  } else if (jp_phase==3) {
    const char *msg="PERDU !";
    int mw=txt_w(msg,10);
    eadk_display_draw_string(msg,(eadk_point_t){160-mw/2,172},true,C_RED,C_BG);
    char nbuf[24]; int nn=0;
    const char *np="C'etait ";
    while (*np) nbuf[nn++]=*np++;
    char tmp[8]; int_to_str(jp_secret,tmp);
    const char *tp=tmp; while (*tp) nbuf[nn++]=*tp++;
    nbuf[nn]=0;
    int nw=txt_w(nbuf,7);
    eadk_display_draw_string(nbuf,(eadk_point_t){160-nw/2,195},false,C_GRAY,C_BG);
  }
  const char *h;
  if (jp_phase==0) h="OK valider    BACK sortir";
  else if (jp_phase==1) h="Tape un nombre...";
  else h="OK rejouer    BACK sortir";
  int hw=txt_w(h,7);
  eadk_display_draw_string(h,(eadk_point_t){160-hw/2,218},false,C_GRAY,C_BG);
}

static int jp_play(void) {
  jp_secret=(int)(rng_next()%1000)+1;
  jp_tries=0; jp_input[0]=0; jp_input_len=0; jp_phase=0; jp_hint=0;
  jp_draw();
  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev; prev = k;
#define JHIT(key) ((hit >> (key)) & 1)
    if (JHIT(KEY_HOME) || JHIT(KEY_ONOFF)) return 0;
    if (jp_phase==0 || jp_phase==1) {
      for (int d=0; d<10; d++) {
        if (JHIT(digit_keys[d]) && jp_input_len<4) {
          if (jp_phase==1) { jp_phase=0; jp_input_len=0; jp_input[0]=0; }
          jp_input[jp_input_len++]=(char)('0'+d);
          jp_input[jp_input_len]=0;
          jp_draw();
        }
      }
      if (JHIT(KEY_BACKSPACE)) {
        if (jp_phase==1) { jp_phase=0; jp_input_len=0; jp_input[0]=0; jp_draw(); }
        else if (jp_input_len>0) { jp_input[--jp_input_len]=0; jp_draw(); }
      }
      if (JHIT(KEY_BACK)) return 1;
      if ((JHIT(KEY_OK)||JHIT(KEY_EXE)) && jp_input_len>0) {
        int guess=(int)parse_float(jp_input);
        jp_tries++;
        if (guess==jp_secret) jp_phase=2;
        else if (jp_tries>=10) jp_phase=3;
        else { jp_phase=1; jp_hint=(guess<jp_secret)?1:2; }
        jp_input_len=0; jp_input[0]=0;
        jp_draw();
      }
    } else {
      if (JHIT(KEY_BACK)) return 1;
      if (JHIT(KEY_OK)||JHIT(KEY_EXE)) {
        jp_secret=(int)(rng_next()%1000)+1;
        jp_tries=0; jp_input[0]=0; jp_input_len=0; jp_phase=0;
        jp_draw();
      }
    }
    for (volatile int i=0;i<5000;i++) { }
  }
}

/* ============ SIMON ============ */
#define SIMON_MAX 30
static uint8_t simon_seq[SIMON_MAX];
static int simon_len=0, simon_show_pos=0, simon_input_pos=0;
static int simon_phase=0;   /* 0=affichage, 1=saisie, 2=gameover */
static int simon_timer=0, simon_flash=-1, simon_round_score=0;

/* 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT */
static const uint16_t simon_dim[4] = {
  RGB(0x80, 0x20, 0x20), RGB(0x20, 0x40, 0x80),
  RGB(0x20, 0x60, 0x30), RGB(0x80, 0x60, 0x20)
};
static const uint16_t simon_bright[4] = {
  RGB(0xFF, 0x50, 0x50), RGB(0x50, 0x90, 0xFF),
  RGB(0x50, 0xD0, 0x70), RGB(0xFF, 0xD0, 0x50)
};

/* zones : renvoie x,y,w,h pour un id 0..3 */
static void simon_zone(int id, int *x, int *y, int *w, int *h) {
  if (id == 0) { *x=80; *y=50; *w=160; *h=55; }
  else if (id == 1) { *x=175; *y=115; *w=130; *h=60; }
  else if (id == 2) { *x=80; *y=180; *w=160; *h=55; }
  else { *x=15; *y=115; *w=130; *h=60; }
}

static void simon_draw(void) {
  clear(0,0,320,240,C_BG);
  draw_status_bar("Simon");
  /* score en haut */
  char sbuf[20]; int sn=0;
  const char *pref="Round : ";
  while (*pref) sbuf[sn++]=*pref++;
  sbuf[sn++]=(char)('0'+(simon_len/10));
  sbuf[sn++]=(char)('0'+(simon_len%10));
  sbuf[sn]=0;
  int sw=txt_w(sbuf,7);
  eadk_display_draw_string(sbuf,(eadk_point_t){160-sw/2,22},false,C_TEXT,C_BG);

  /* dessine les 4 zones */
  for (int i=0; i<4; i++) {
    int x,y,w,h;
    simon_zone(i,&x,&y,&w,&h);
    uint16_t col = (simon_flash == i) ? simon_bright[i] : simon_dim[i];
    clear(x,y,w,h,col);
    clear(x,y,w,2,C_WHITE); clear(x,y+h-2,w,2,C_WHITE);
    clear(x,y,2,h,C_WHITE); clear(x+w-2,y,2,h,C_WHITE);
  }

  /* texte bas */
  const char *h;
  if (simon_phase==0) h="Regarde...";
  else if (simon_phase==1) h="A toi ! (fleches)";
  else h="GAME OVER - OK rejouer";
  int hw=txt_w(h,7);
  eadk_display_draw_string(h,(eadk_point_t){160-hw/2,225},false,C_GRAY,C_BG);
}

/* Convertit une touche en id de zone, -1 sinon */
static int simon_key_to_id(uint64_t hit) {
  if ((hit >> KEY_UP) & 1) return 0;
  if ((hit >> KEY_RIGHT) & 1) return 1;
  if ((hit >> KEY_DOWN) & 1) return 2;
  if ((hit >> KEY_LEFT) & 1) return 3;
  return -1;
}

static void simon_new_round(void) {
  simon_seq[simon_len] = (uint8_t)(rng_next() % 4);
  simon_len++;
  simon_show_pos = 0;
  simon_input_pos = 0;
  simon_timer = 0;
  simon_flash = -1;
  simon_phase = 0;
}

static int simon_play(void) {
  simon_len = 0;
  simon_phase = 0;
  simon_flash = -1;
  simon_new_round();
  simon_draw();

  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev; prev = k;
#define SHIT(key) ((hit >> (key)) & 1)
    if (SHIT(KEY_HOME) || SHIT(KEY_ONOFF)) return 0;

    if (simon_phase == 0) {
      /* animation : allume puis eteint chaque element */
      simon_timer++;
      if (simon_flash == -1) {
        /* allume l'element courant */
        if (simon_show_pos < simon_len) {
          simon_flash = simon_seq[simon_show_pos];
          simon_draw();
        }
      } else {
        /* attend un peu avant d'eteindre */
        if (simon_timer > 30) {
          simon_flash = -1;
          simon_timer = 0;
          simon_show_pos++;
          simon_draw();
          if (simon_show_pos >= simon_len) {
            /* fin de l'affichage : passe a la saisie */
            simon_phase = 1;
            simon_input_pos = 0;
            simon_draw();
          }
        }
      }
    } else if (simon_phase == 1) {
      int id = simon_key_to_id(hit);
      if (id >= 0) {
        /* flash rapide */
        simon_flash = id;
        simon_draw();
        /* verifie */
        if (id == simon_seq[simon_input_pos]) {
          simon_input_pos++;
          if (simon_input_pos >= simon_len) {
            /* round reussi */
            simon_phase = 0;
            simon_show_pos = 0;
            simon_timer = 0;
            simon_flash = -1;
            simon_new_round();
            simon_draw();
          }
        } else {
          /* erreur */
          simon_phase = 2;
          simon_flash = -1;
          simon_draw();
        }
      }
      if (SHIT(KEY_BACK)) return 1;
    } else {
      /* gameover */
      if (SHIT(KEY_BACK)) return 1;
      if (SHIT(KEY_OK) || SHIT(KEY_EXE)) {
        simon_len = 0;
        simon_phase = 0;
        simon_flash = -1;
        simon_new_round();
        simon_draw();
      }
    }

    /* reset flash si on est dans la phase 1 (saisie) */
    if (simon_phase == 1 && simon_flash >= 0) {
      /* attends un peu puis eteint */
      static int flash_timer = 0;
      flash_timer++;
      if (flash_timer > 5) { simon_flash = -1; simon_draw(); flash_timer = 0; }
    }

    for (volatile int i=0;i<3000;i++) { }
  }
}

/* ============ MAIN ============ */
int main(void) {
  saisie[0]=0; saisie_len=0;
  src_cur=0; dst_cur=1; resultat=0.0f;
  mode=0; seq_pos=0; nexus_sel=0;
  rng_state=12345;
  draw_devises();

  uint64_t prev = eadk_keyboard_scan();
  while (1) {
    uint64_t k = eadk_keyboard_scan();
    uint64_t hit = k & ~prev; prev = k;
    if (hit == 0) { for (volatile int i=0;i<10000;i++) { } continue; }
#define HIT(key) ((hit >> (key)) & 1)
    if (HIT(KEY_HOME) || HIT(KEY_ONOFF)) return 0;

    /* Konami */
    if (mode==0 && hit && (hit & (hit-1))==0) {
      int pressed=-1;
      for (int i=0;i<64;i++) if (hit & (1ULL<<i)) { pressed=i; break; }
      if (pressed>=0) {
        if (pressed == SEQ[seq_pos]) {
          seq_pos++;
          if (seq_pos == SEQ_LEN) {
            mode=1; seq_pos=0; nexus_sel=0;
            draw_nexus();
            continue;
          }
        } else {
          seq_pos = (pressed == SEQ[0]) ? 1 : 0;
        }
      }
    }

    if (mode==0) {
      if (HIT(KEY_LEFT)) { src_cur=(src_cur+N_CUR-1)%N_CUR; recalc(); refresh_values(); }
      if (HIT(KEY_RIGHT)) { src_cur=(src_cur+1)%N_CUR; recalc(); refresh_values(); }
      if (HIT(KEY_UP)) { dst_cur=(dst_cur+N_CUR-1)%N_CUR; recalc(); refresh_values(); }
      if (HIT(KEY_DOWN)) { dst_cur=(dst_cur+1)%N_CUR; recalc(); refresh_values(); }
      for (int d=0;d<10;d++) {
        if (HIT(digit_keys[d]) && saisie_len<9) {
          saisie[saisie_len++]=(char)('0'+d); saisie[saisie_len]=0;
          recalc(); refresh_values();
        }
      }
      if (HIT(KEY_DOT) && saisie_len<9) {
        int has=0;
        for (int i=0;i<saisie_len;i++) if (saisie[i]=='.') { has=1; break; }
        if (!has) {
          if (!saisie_len) saisie[saisie_len++]='0';
          saisie[saisie_len++]='.'; saisie[saisie_len]=0;
          recalc(); refresh_values();
        }
      }
      if (HIT(KEY_BACKSPACE) && saisie_len>0) {
        saisie[--saisie_len]=0; recalc(); refresh_values();
      }
    }
    else if (mode==1) {
      if (HIT(KEY_LEFT)) { nexus_sel=(nexus_sel+3)%4; draw_nexus(); }
      if (HIT(KEY_RIGHT)) { nexus_sel=(nexus_sel+1)%4; draw_nexus(); }
      if (HIT(KEY_BACK)) { mode=0; seq_pos=0; draw_devises(); continue; }
      if (HIT(KEY_OK) || HIT(KEY_EXE)) {
        if (nexus_sel==0) { rng_state+=7; int r=pfc_play(); if(!r) return 0; mode=1; draw_nexus(); }
        else if (nexus_sel==1) { rng_state+=13; int r=jp_play(); if(!r) return 0; mode=1; draw_nexus(); }
        else if (nexus_sel==2) { rng_state+=19; int r=simon_play(); if(!r) return 0; mode=1; draw_nexus(); }
        else if (nexus_sel==3) { mode=0; seq_pos=0; draw_devises(); }
      }
    }
  }
  return 0;
}
