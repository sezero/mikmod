#define MAXOBJECTS 100

typedef struct RBSUIObject {
  void (*click)(struct RBSUIObject *obj,int x,int y);
  void (*down)(struct RBSUIObject *obj,int x,int y);
  void (*up)(struct RBSUIObject *obj,int x,int y);
  void (*draw)(struct RBSUIObject *obj);
  void (*destroy)(struct RBSUIObject *obj);
  int x,y,w,h;
  void *data;
  int num;
  int user_id;
} RBSUIObject;

int RBSUI_Init(SDL_Surface *s,void (*quit)(),char *f);
void RBSUI_Quit();
void RBSUI_DestroyObject(RBSUIObject *obj);
void RBSUI_DestroyAllObjects();
RBSUIObject *RBSUI_CreateButton(int x,int y,int w,int h,char *caption, 
				void (*action)(RBSUIObject *obj,int x,int y));
RBSUIObject *RBSUI_CreateList(int x,int y,int w,int h,char **item, 
			      int items,int selected,int scrolloffset);
RBSUIObject *RBSUI_CreateSlider(int x,int y,int w,int h,char *caption, 
				int min,int max,int value,
				void (*change)(RBSUIObject *obj,int value));
RBSUIObject *RBSUI_CreateLabel(int x,int y,int w,int h,char *text);

void RBSUI_SliderSet(RBSUIObject *obj,int value);
int RBSUI_SliderGet(RBSUIObject *obj);

void RBSUI_Draw();
void RBSUI_Update(RBSUIObject *obj);
void RBSUI_Poll();

typedef struct RBSUIButtonData {
  char *caption;
  int pushed;
} RBSUIButtonData;

typedef struct RBSUILabelData {
  char *text;
} RBSUILabelData;

typedef struct RBSUISliderData {
  char *caption;
  int value;
  int min;
  int max;
  void (*change)(RBSUIObject *obj,int value);
} RBSUISliderData;

typedef struct RBSUIListData {
  char **item;
  int items;
  int selected;
  int scrolloffset;
} RBSUIListData;
