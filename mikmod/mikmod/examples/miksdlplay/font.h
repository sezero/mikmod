#define CHARSET_SIZE 256

typedef struct RBSFont {
  SDL_Surface *bitmap[CHARSET_SIZE];
  int height;
} RBSFont;

RBSFont *RBSFont_Load(char *filename,int firstChar);
void RBSFont_Destroy(RBSFont *font);
void RBSFont_Puts(SDL_Surface *screen,int x,int y,RBSFont *font,unsigned char *s);
void RBSFont_Printf(SDL_Surface *screen,int x,int y,RBSFont *font,char *format, ...);
