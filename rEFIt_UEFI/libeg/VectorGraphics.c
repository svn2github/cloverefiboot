/*
 * Additional procedures for vector theme support
 *
 * Slice, 2018
 *
 */


#include "nanosvg.h"
#include "FloatLib.h"

#include "lodepng.h"

#ifndef DEBUG_ALL
#define DEBUG_VEC 1
#else
#define DEBUG_VEC DEBUG_ALL
#endif

#if DEBUG_VEC == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_VEC, __VA_ARGS__)
#endif

#define TEST_MATH 0
#define TEST_SVG_IMAGE 1
#define TEST_SIZEOF 0
#define TEST_FONT 0
#define TEST_DITHER 1

#define NSVG_RGB(r, g, b) (((unsigned int)b) | ((unsigned int)g << 8) | ((unsigned int)r << 16))
//#define NSVG_RGBA(r, g, b, a) (((unsigned int)b) | ((unsigned int)g << 8) | ((unsigned int)r << 16) | ((unsigned int)a << 24))

extern VOID
WaitForKeyPress(CHAR16 *Message);

extern void DumpFloat2 (char* s, float* t, int N);
extern EG_IMAGE *BackgroundImage;
extern EG_IMAGE *Banner;
extern EG_IMAGE *BigBack;
extern VOID *fontsDB;
extern INTN BanHeight;
extern INTN row0TileSize;
extern INTN row1TileSize;
extern INTN FontWidth;

textFaces textFace[4]; //0-help 1-message 2-menu 3-test


EG_IMAGE  *ParseSVGIcon(NSVGparser  *p, INTN Id, CHAR8 *IconName, float Scale)
{
//  EFI_STATUS      Status = EFI_NOT_FOUND;
  NSVGimage       *SVGimage;
  NSVGrasterizer* rast = nsvgCreateRasterizer();
  SVGimage = p->image;
  NSVGshape   *shape;
  NSVGgroup   *group;
  NSVGimage *IconImage; // = (NSVGimage*)AllocateZeroPool(sizeof(NSVGimage));
  NSVGshape *shapeNext, *shapesTail=NULL, *shapePrev;

  NSVGparser* p2 = nsvg__createParser();
  IconImage = p2->image;

  shape = SVGimage->shapes;
  shapePrev = NULL;
  while (shape) {
    group = shape->group;

    shapeNext = shape->next;

      while (group) {
        if (strcmp(group->id, IconName) == 0) {
          break;
        }
        group = group->next;
      }

    if (group) { //the shape is in the group
/*      DBG("found shape %a", shape->id);
      DBG(" from group %a\n", group->id);
      if ((Id == BUILTIN_SELECTION_BIG) ||
          (Id == BUILTIN_ICON_BACKGROUND) ||
          (Id == BUILTIN_ICON_BANNER)) {
        shape->debug = TRUE;
      } */
      if (strstr(shape->id, "BoundingRect") != NULL) {
        //there is bounds after nsvgParse()
        IconImage->width = shape->bounds[2] - shape->bounds[0];
        IconImage->height = shape->bounds[3] - shape->bounds[1];

  //      memcpy(IconImage->realBounds, shape->bounds, 4*sizeof(float));
        if (!IconImage->height) {
          IconImage->height = 200;
        }
 //       if (Id == BUILTIN_ICON_BACKGROUND || Id == BUILTIN_ICON_BANNER) {
 //         DBG("IconImage size [%d,%d]\n", (int)IconImage->width, (int)IconImage->height);
 //         DBG("IconImage left corner x=%s y=%s\n", PoolPrintFloat(IconImage->realBounds[0]), PoolPrintFloat(IconImage->realBounds[1]));
 //         DumpFloat2("IconImage real bounds", IconImage->realBounds, 4);
 //       }
        if ((Id == BUILTIN_SELECTION_BIG) && (!GlobalConfig.SelectionOnTop)) {
          GlobalConfig.MainEntriesSize = (int)(IconImage->width * Scale); //xxx
          row0TileSize = GlobalConfig.MainEntriesSize; // + (int)(16.f * Scale);
          DBG("main entry size = %d\n", GlobalConfig.MainEntriesSize);
        }
   //     GlobalConfig.SelectionOnTop
   //     row0TileSize = (INTN)(144.f * Scale);
        if ((Id == BUILTIN_SELECTION_SMALL) && (!GlobalConfig.SelectionOnTop)) {
          row1TileSize = (int)(IconImage->width * Scale);
        }

        shape->flags = 0;  //invisible
        if (shapePrev) {
          shapePrev->next = shapeNext;
        } else {
          SVGimage->shapes = shapeNext;
        }
        shape = shapeNext;
//        Status = EFI_SUCCESS;
        continue; //while(shape) it is BoundingRect shape
      }
      shape->flags = NSVG_VIS_VISIBLE;
//      DBG("shape opacity=%s, fill color=0x%x\n", PoolPrintFloat(shape->opacity), shape->fill.color);
      //should be added to tail
      // Add to tail
      if (IconImage->shapes == NULL)
        IconImage->shapes = shape;
      else
        shapesTail->next = shape;
      shapesTail = shape;
      if (shapePrev) {
        shapePrev->next = shapeNext;
      } else {
        SVGimage->shapes = shapeNext;
      }
      shapePrev->next = shapeNext;
    } //the shape in the group
    else {
      shapePrev = shape;
    }
    shape = shapeNext;
  } //while shape
  shapesTail->next = NULL;

  //add clipPaths  //xxx
  NSVGclipPath* clipPaths = SVGimage->clipPaths;
  NSVGclipPath* clipNext = NULL;
  while (clipPaths) {
    group = clipPaths->shapes->group;
    clipNext = clipPaths->next;
    while (group) {
      if (strcmp(group->id, IconName) == 0) {
        break;
      }
      group = group->parent;
    }
    if (group) {
      DBG("found clipPaths for %a\n", IconName);
      IconImage->clipPaths = clipPaths;
    }
    clipPaths = clipNext;
  }

  float bounds[4];
  bounds[0] = FLT_MAX;
  bounds[1] = FLT_MAX;
  bounds[2] = -FLT_MAX;
  bounds[3] = -FLT_MAX;
  nsvg__imageBounds(p2, bounds);
//  DumpFloat2("p2 image bounds", bounds, 4);
  // Patch: save real bounds.
  memcpy(IconImage->realBounds, bounds, 4*sizeof(float));
/*
  if (strstr(IconName, "vol_internal") != NULL) {
    DBG("icon=%a ", IconName);
    DumpFloat2(" ", bounds, 4);
  }
*/
  if ((Id == BUILTIN_ICON_BANNER) && (strcmp(IconName, "Banner") == 0)) {
    GlobalConfig.BannerPosX = (int)(bounds[0] * Scale - GlobalConfig.CentreShift);
    GlobalConfig.BannerPosY = (int)(bounds[1] * Scale);
    DBG("Banner position at parse [%d,%d]\n", GlobalConfig.BannerPosX, GlobalConfig.BannerPosY);
  }

  float Height = IconImage->height * Scale;
  float Width = IconImage->width * Scale;
//  DBG("icon %a width=%s height=%s\n", IconName, PoolPrintFloat(Width), PoolPrintFloat(Height));
  EG_IMAGE  *NewImage = NULL;
  int iWidth = (int)(Width + 0.5f);
  int iHeight = (int)(Height + 0.5f);

//  DBG("begin rasterize %a\n", IconName);
  float tx = 0.f, ty = 0.f;
//  if (Id == BUILTIN_ICON_BACKGROUND) {
//    tx = - GlobalConfig.CentreShift;
//    IconImage->realBounds[0] += tx;
//    IconImage->realBounds[2] += tx;
//    iWidth = (int)UGAWidth;
//  } else
  if ((Id != BUILTIN_ICON_BACKGROUND) && (strcmp(IconName, "Banner") != 0)) {
    float realWidth = (bounds[2] - bounds[0]) * Scale;
    float realHeight = (bounds[3] - bounds[1]) * Scale;
    tx = (Width - realWidth) * 0.5f;
    ty = (Height - realHeight) * 0.5f;
  }
  NewImage = egCreateFilledImage(iWidth, iHeight, TRUE, &MenuBackgroundPixel);
  nsvgRasterize(rast, IconImage, tx,ty,Scale,Scale, (UINT8*)NewImage->PixelData, iWidth, iHeight, iWidth*4, NULL, NULL);
//  DBG("%a rastered, blt\n", IconImage);
#if 0
  BltImageAlpha(NewImage,
                (int)(UGAWidth - NewImage->Width) / 2,
                (int)(UGAHeight * 0.05f),
                &MenuBackgroundPixel,
                16);
//  WaitForKeyPress(L"waiting for key press...\n");
#endif
  nsvgDeleteRasterizer(rast);
//  nsvg__deleteParser(p2);
//  nsvgDelete(p2->image);
  return NewImage;
}

EFI_STATUS ParseSVGTheme(CONST CHAR8* buffer, TagPtr * dict, UINT32 bufSize)
{

  NSVGparser      *p = NULL;
  NSVGfont        *fontSVG;
  NSVGimage       *SVGimage;
  NSVGrasterizer  *rast = nsvgCreateRasterizer();
  EFI_TIME          Now;
  gRT->GetTime(&Now, NULL);
  INT32 NowHour = Now.Hour + GlobalConfig.Timezone;
  BOOLEAN DayLight = (NowHour > 8) && (NowHour < 20);


// --- Parse Theme.svg
  p = nsvgParse((CHAR8*)buffer, "px", 72, 1.f);
  SVGimage = p->image;
  if (!SVGimage) {
    DBG("Theme not parsed!\n");
    return EFI_NOT_STARTED;
  }

// --- Get scale as theme design height vs screen height
  float Scale;
  // must be svg view-box
  float vbx = p->viewWidth;
  float vby = p->viewHeight;
  DBG("Theme view-bounds: w=%d h=%d units=%a\n", (int)vbx, (int)vby, "px");
    if (vby > 1.0f) {
      SVGimage->height = vby;
    } else {
      SVGimage->height = 768.f;  //default height
    }
  Scale = UGAHeight / SVGimage->height;
  DBG("using scale %s\n", PoolPrintFloat(Scale));
  GlobalConfig.Scale = Scale;
  GlobalConfig.CentreShift = (vbx * Scale - (float)UGAWidth) * 0.5f;

// --- Get theme embedded font (already parsed above)
  fontSVG = p->font;
  if (!fontSVG) {
    fontSVG = fontsDB;
    while (fontSVG && !fontSVG->glyphs) {
	    fontSVG = fontSVG->next;
    }
  }
  if (fontSVG) {
    DBG("theme contains font-family=%a\n", fontSVG->fontFamily);
  }
#if 0
// --- Create rastered font
  if (fontSVG) {
    if (p->font) {
      FontHeight = (int)(textFace[2].size * Scale); //as in MenuRows
      DBG("Menu font scaled height=%d color=%x\n", FontHeight, textFace[2].color);
    }
    if (!FontHeight) FontHeight = 16;  //xxx
    if (fontSVG->fontFamily[0] < 0x30) {
      AsciiStrCpyS(fontSVG->fontFamily, 64, fontSVG->id);
    }
    RenderSVGfont(fontSVG, p->fontColor);
    DBG("font %a parsed\n", fontSVG->fontFamily);
  }
#endif
// --- Make background
  BackgroundImage = egCreateFilledImage(UGAWidth, UGAHeight, TRUE, &MenuBackgroundPixel);
  if (DayLight) {
    BigBack = ParseSVGIcon(p, BUILTIN_ICON_BACKGROUND, "Background", Scale);
  } else {
    BigBack = ParseSVGIcon(p, BUILTIN_ICON_BACKGROUND, "Background_night", Scale);
  }

//  GlobalConfig.BackgroundScale = imScale;

// --- Make Banner
  Banner = ParseSVGIcon(p, BUILTIN_ICON_BANNER, "Banner", Scale);
  BuiltinIconTable[BUILTIN_ICON_BANNER].Image = Banner;
  BanHeight = (int)(Banner->Height * Scale + 1.f);
  DBG("parsed banner->width=%d\n", Banner->Width);

// --- Make other icons

  INTN i = BUILTIN_ICON_FUNC_ABOUT;
  CHAR8           *IconName;
  while (BuiltinIconTable[i].Path) {
    CHAR16 *IconPath = BuiltinIconTable[i].Path;
//    DBG("next table icon=%s\n", IconPath);
    CHAR16 *ptr = StrStr(IconPath, L"\\");
    if (!ptr) {
      ptr = IconPath;
    } else {
      ptr++;
    }
 //   DBG("next icon=%s Len=%d\n", ptr, StrLen(ptr));
    UINTN Size = StrLen(ptr)+1;
    IconName = AllocateZeroPool(Size);
    UnicodeStrToAsciiStrS(ptr, IconName, Size);
//    DBG("search for icon name %a\n", IconName);

    BuiltinIconTable[i].Image = ParseSVGIcon(p, i, IconName, Scale);
    if (!BuiltinIconTable[i].Image) {
      DBG(" icon %d not parsed\n", i);
    }
    if (i == BUILTIN_SELECTION_BIG) {
      DBG("icon main size=[%d,%d]\n", BuiltinIconTable[i].Image->Width,
          BuiltinIconTable[i].Image->Height);
    }
    i++;
    FreePool(IconName);
  }

  // OS icons and buttons
  i = 0;
  while (OSIconsTable[i].name) {
//    DBG("search for %a\n", OSIconsTable[i].name);
    if ((strcmp(OSIconsTable[i].name, "os_moja") == 0) && !DayLight) {
      OSIconsTable[i].image = ParseSVGIcon(p, i, "os_moja_night", Scale);
    } else {
      OSIconsTable[i].image = ParseSVGIcon(p, i, OSIconsTable[i].name, Scale);
    }
    if (OSIconsTable[i].image == NULL) {
      DBG("OSicon %a not parsed\n", OSIconsTable[i].name);
    }
    i++;
  }

  if (p) {
//    nsvg__deleteParser(p);
    p = NULL;
  }

  nsvgDeleteRasterizer(rast);

  *dict = AllocateZeroPool(sizeof(TagStruct));
  (*dict)->type = kTagTypeNone;
  GlobalConfig.TypeSVG = TRUE;
  GlobalConfig.ThemeDesignHeight = (int)SVGimage->height;
  GlobalConfig.ThemeDesignWidth = (int)SVGimage->width;
  if (GlobalConfig.SelectionOnTop) {
    row0TileSize = (INTN)(144.f * Scale);
    row1TileSize = (INTN)(64.f * Scale);
    GlobalConfig.MainEntriesSize = (INTN)(128.f * Scale);
  }
  DBG("parsing theme finish\n");
  return EFI_SUCCESS;
}

VOID RenderSVGfont(NSVGfont  *fontSVG, UINT32 color)
{
//  EFI_STATUS      Status;
  float           FontScale;
  NSVGparser      *p;
  NSVGrasterizer  *rast;
  INTN i;
  if (!fontSVG) {
    return;
  }
  //free old font
  if (FontImage != NULL) {
    egFreeImage (FontImage);
    FontImage = NULL;
  }
  INTN Height = FontHeight + 4;
//  DBG("load font %a\n", fontSVG->fontFamily);
  if (fontSVG->unitsPerEm < 1.f) {
    fontSVG->unitsPerEm = 1000.f;
  }
  float fH = fontSVG->bbox[3] - fontSVG->bbox[1];
  if (fH == 0.f) {
    fH = fontSVG->unitsPerEm;
  }
  FontScale = (float)FontHeight / fH;
  DBG("font scale %s\n", PoolPrintFloat(FontScale));
  FontWidth = (int)(fontSVG->horizAdvX * FontScale);
  INTN Width = FontWidth * (AsciiPageSize + GlobalConfig.CodepageSize);
  FontImage = egCreateImage(Width, Height, TRUE);

  p = nsvg__createParser();
  if (!p) {
    return;
  }
//  p->font = fontSVG;
  p->image->height = (float)Height;
  p->image->width = (float)Width;

  NSVGtext* text = (NSVGtext*)AllocateZeroPool(sizeof(NSVGtext));
  if (!text) {
    return;
  }
  text->fontSize = (float)FontHeight;
  text->font = fontSVG;
  text->fontColor = color;

//  DBG("RenderSVGfont: fontID=%a\n", text->font->id);
//  DBG("RenderSVGfont:  family=%a\n", text->font->fontFamily);
  //add to head
  text->next = p->text;
  p->text = text;
  //for each letter rasterize glyph into FontImage
  //0..0xC0 == AsciiPageSize
  // cyrillic 0x410..0x450 at 0xC0
  float x = 0.f;
  float y = fontSVG->bbox[1] * FontScale;; //(float)Height;
  p->isText = TRUE;
  for (i = 0; i < AsciiPageSize; i++) {
    addLetter(p, i, x, y, FontScale, color);
    x += (float)FontWidth;
  }
  x = AsciiPageSize * FontWidth;
  for (i = GlobalConfig.Codepage; i < GlobalConfig.Codepage+GlobalConfig.CodepageSize; i++) {
    addLetter(p, i, x, y, FontScale, color);
    x += (float)FontWidth;
  }
  p->image->realBounds[0] = fontSVG->bbox[0] * FontScale;
  p->image->realBounds[1] = fontSVG->bbox[1] * FontScale;
  p->image->realBounds[2] = fontSVG->bbox[2] * FontScale + x; //last bound
  p->image->realBounds[3] = fontSVG->bbox[3] * FontScale;

  //We made an image, then rasterize it
  rast = nsvgCreateRasterizer();
  nsvgRasterize(rast, p->image, 0, 0, 1.0f, 1.0f, (UINT8*)FontImage->PixelData, (int)Width, (int)Height, (int)(Width*4), NULL, NULL);

#if 0 //DEBUG_FONT
  //save font as png yyyyy
  UINT8           *FileData = NULL;
  UINTN           FileDataLength = 0U;

  EFI_UGA_PIXEL *ImagePNG = (EFI_UGA_PIXEL *)FontImage->PixelData;

  unsigned lode_return =
    eglodepng_encode(&FileData, &FileDataLength, (CONST UINT8*)ImagePNG, (UINTN)FontImage->Width, (UINTN)FontImage->Height);

  if (!lode_return) {
    egSaveFile(SelfRootDir, L"\\FontSVG.png", FileData, FileDataLength);
  }
#endif
  nsvgDeleteRasterizer(rast);
//  nsvg__deleteParser(p);
  return;
}

//textType = 0-help 1-message 2-menu 3-test
//return text width in pixels
INTN drawSVGtext(EG_IMAGE* TextBufferXY, INTN posX, INTN posY, INTN textType, CONST CHAR16* string, INTN Cursor)
{
  INTN Width;
  int i;
  UINTN len;
  NSVGparser* p;
  NSVGrasterizer* rast;
  if (!textFace[textType].valid) {
    for (i=0; i<4; i++) {
      if (textFace[i].valid) {
        textType = i;
        break;
      }
    }
  }
  if (!textFace[textType].valid) {
    DBG("valid fontface not found!\n");
    return 0;
  }
  NSVGfont* fontSVG = textFace[textType].font;
  UINT32 color = textFace[textType].color;
  INTN Height = (INTN)(textFace[textType].size * GlobalConfig.Scale);
  float Scale, sy;
  float x, y;
  if (!fontSVG) {
    DBG("no font for drawSVGtext\n");
    return 0;
  }
  if (!TextBufferXY) {
    DBG("no buffer\n");
    return 0;
  }
  p = nsvg__createParser();
  if (!p) {
    return 0;
  }
  NSVGtext* text = (NSVGtext*)AllocateZeroPool(sizeof(NSVGtext));
  if (!text) {
    return 0;
  }
  text->font = fontSVG;
  text->fontColor = color;
  text->fontSize = Height;
  p->text = text;

  len = StrLen(string);
  Width = TextBufferXY->Width;
//  Height = TextBufferXY->Height;
//  DBG("Text Height=%d  Buffer Height=%d\n", Height, TextBufferXY->Height);

//  Height = 180; //for test
//  DBG("textBuffer: [%d,%d], fontUnits=%d\n", Width, TextBufferXY->Height, (int)fontSVG->unitsPerEm);
  if (!fontSVG->unitsPerEm) {
    fontSVG->unitsPerEm = 1000.f;
  }
  float fH = fontSVG->bbox[3] - fontSVG->bbox[1]; //1250
  if (fH == 0.f) {
    DBG("wrong font: %s\n", PoolPrintFloat(fontSVG->unitsPerEm));
    DumpFloat2("Font bbox", fontSVG->bbox, 4);
    fH = fontSVG->unitsPerEm?fontSVG->unitsPerEm:1000.0f;  //1000
  }
  sy = (float)Height / fH; //(float)fontSVG->unitsPerEm; // 260./1250.
  //in font units
//  float fW = fontSVG->bbox[2] - fontSVG->bbox[0];
//  sx = (float)Width / (fW * len);
//  Scale = (sx > sy)?sy:sx;
  Scale = sy;
  x = (float)posX; //0.f;
  y = (float)posY + fontSVG->bbox[1] * Scale;
  p->isText = TRUE;
  for (i=0; i < len; i++) {
    CHAR16 letter = string[i]; //already UTF16
//    string = GetUnicodeChar(string, &letter);
    if (!letter) {
      break;
    }
//    DBG("add letter 0x%x\n", letter);
    if (i == Cursor) {
      addLetter(p, 0x5F, x, y, sy, color);
    }
    x = addLetter(p, letter, x, y, sy, color);
//    DBG("next x=%s\n", PoolPrintFloat(x));
  } //end of string

  p->image->realBounds[0] = fontSVG->bbox[0] * Scale;
  p->image->realBounds[1] = fontSVG->bbox[1] * Scale;
  p->image->realBounds[2] = fontSVG->bbox[2] * Scale + x; //last bound
  p->image->realBounds[3] = fontSVG->bbox[3] * Scale;
//  DBG("internal Scale=%s\n", PoolPrintFloat(Scale));
//  DumpFloat2("text bounds", p->image->realBounds, 4);
  //We made an image, then rasterize it
  rast = nsvgCreateRasterizer();
//  DBG("begin raster text, scale=%s\n", PoolPrintFloat(Scale));
  nsvgRasterize(rast, p->image, 0, 0, 1.f, 1.f, (UINT8*)TextBufferXY->PixelData,
                (int)TextBufferXY->Width, (int)TextBufferXY->Height, (int)(Width*4), NULL, NULL);
  float RealWidth = p->image->realBounds[2] - p->image->realBounds[0];
//  DBG("end raster text\n");
  nsvgDeleteRasterizer(rast);
//  nsvg__deleteParser(p);
  nsvgDelete(p->image);
  return (INTN)RealWidth; //x;
}

VOID testSVG()
{
  do {

    EFI_STATUS      Status;
    UINT8           *FileData = NULL;
    UINTN           FileDataLength = 0;

    INTN Width = 400, Height = 400;

#if TEST_MATH
    //Test mathematique
//#define fabsf(x) ((x >= 0.0f)?x:(-x))
#define pr(x) (int)fabsf(x), (int)fabsf((x - (int)x) * 1000000.0f)
    int i;
    float x, y1, y2;
    //          CHAR8 Str[128];
    DBG("Test float: -%d.%06d\n", pr(-0.7612f));
    for (i=0; i<15; i++) {
      x=(PI)/30.0f*i;
      y1=SinF(x);
      y2=CosF(x);

      DBG("x=%d: %d.%06d ", i*6, pr(x));
      DBG("  sinx=%c%d.%06d", (y1<0)?'-':' ', pr(y1));
      DBG("  cosx=%c%d.%06d\n", (y2<0)?'-':' ',pr(y2));
      y1 = Atan2F(y1, y2);
      DBG("  atan2x=%c%d.%06d", (y1<0)?'-':' ',pr(y1));
      y1 = AcosF(y2);
      DBG("  acos=%c%d.%06d", (y1<0)?'-':' ',pr(y1));
      y1 = SqrtF(x);
      DBG("  sqrt=%d.%06d", pr(y1));
      y1 = CeilF(x);
      DBG("  ceil=%c%d.%06d\n", (y1<0)?'-':' ',pr(y1));
    }
#undef pr
#endif
    NSVGparser* p;
#if TEST_DITHER
    {
      EG_IMAGE        *RndImage = egCreateImage(256, 256, TRUE);
      INTN i,j;
      EG_PIXEL pixel = WhitePixel;
      for (i=0; i<256; i++) {
        for (j=0; j<256; j++) {
          pixel.b = 0x40 + (dither((float)j / 32.0f, 1) * 8);
          pixel.r = 0x0;
          pixel.g = 0x0;
//          if (i==1) {
//            DBG("r=%x g=%x\n", pixel.r, pixel.g);
//          }
          RndImage->PixelData[i * 256 + j] = pixel;
        }
      }

      BltImageAlpha(RndImage,
                    20,
                    20,
                    &MenuBackgroundPixel,
                    16);
    }
#endif
#if TEST_SVG_IMAGE
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    EG_IMAGE        *NewImage;
    NSVGimage       *SVGimage;
    float Scale, ScaleX, ScaleY;
    
    // load file
    Status = egLoadFile(SelfRootDir, L"Sample.svg", &FileData, &FileDataLength);
    if (!EFI_ERROR(Status)) {
      //Parse XML to vector data

      p = nsvgParse((CHAR8*)FileData, "px", 72, 0.f);
      SVGimage = p->image;
      DBG("Test image width=%d heigth=%d\n", (int)(SVGimage->width), (int)(SVGimage->height));
      FreePool(FileData);
/*
      if (p->patterns && p->patterns->image) {
        BltImageAlpha((EG_IMAGE*)(p->patterns->image),
                      40,
                      40,
                      &MenuBackgroundPixel,
                      16);
      }
*/
      // Rasterize
      NewImage = egCreateImage(Width, Height, TRUE);
      if (SVGimage->width <= 0) SVGimage->width = Width;
      if (SVGimage->height <= 0) SVGimage->height = Height;

      ScaleX = Width / SVGimage->width;
      ScaleY = Height / SVGimage->height;
      Scale = (ScaleX > ScaleY)?ScaleY:ScaleX;
      float tx = 0; //-SVGimage->realBounds[0] * Scale;
      float ty = 0; //-SVGimage->realBounds[1] * Scale;
      DBG("timing rasterize start tx=%s ty=%s\n", PoolPrintFloat(tx), PoolPrintFloat(ty));
      nsvgRasterize(rast, SVGimage, tx,ty,Scale,Scale, (UINT8*)NewImage->PixelData, (int)Width, (int)Height, (int)Width*4, NULL, NULL);
      DBG("timing rasterize end\n");
      //now show it!
      BltImageAlpha(NewImage,
                    (UGAWidth - Width) / 2,
                    (UGAHeight - Height) / 2,
                    &MenuBackgroundPixel,
                    16);
      egFreeImage(NewImage);
      nsvg__deleteParser(p);
      nsvgDeleteRasterizer(rast);
    }
#endif
    //Test text
    Height = 80;
    Width = UGAWidth-200;
    DBG("create test textbuffer\n");
    EG_IMAGE* TextBufferXY = egCreateFilledImage(Width, Height, TRUE, &MenuBackgroundPixel);
    Status = egLoadFile(SelfRootDir, L"Font.svg", &FileData, &FileDataLength);
    DBG("test Font.svg loaded status=%r\n", Status);
    if (!EFI_ERROR(Status)) {
      p = nsvgParse((CHAR8*)FileData, "px", 72, 1.f);
      if (!p) {
        DBG("font not parsed\n");
        break;
      }
//     NSVGfont* fontSVG = p->font;
      textFace[3].font = p->font;
      textFace[3].color = NSVG_RGBA(0x80, 0xFF, 0, 255);
      textFace[3].size = Height;
//      DBG("font parsed family=%a\n", p->font->fontFamily);
      FreePool(FileData);
      //   Scale = Height / fontSVG->unitsPerEm;
      drawSVGtext(TextBufferXY, 0, 0, 3, L"Clover Кловер", 1);
//      DBG("text ready to blit\n");
      BltImageAlpha(TextBufferXY,
                    (UGAWidth - Width) / 2,
                    (UGAHeight - Height) / 2,
                    &MenuBackgroundPixel,
                    16);
      egFreeImage(TextBufferXY);
//      nsvg__deleteParser(p);
//      DBG("draw finished\n");
    }
  } while (0);

}
