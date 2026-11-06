#include "../src/c/face_settings.h"
#include "../src/c/kit/kit.h"
#include <assert.h>
#include <stdio.h>

static int32_t saved[2];
static int sizes[2], writes;
static TupleValue values[2];
static Tuple tuples[2];
static unsigned present;
int persist_get_size(uint32_t key) { return key>=2000 && key<=2001 ? sizes[key-2000] : -1; }
int32_t persist_read_int(uint32_t key) { assert(key>=2000 && key<=2001);return saved[key-2000]; }
int persist_write_int(uint32_t key,int32_t value) {
  assert(key>=2000 && key<=2001);saved[key-2000]=value;sizes[key-2000]=4;++writes;return 4;
}
int persist_read_data(uint32_t key,void *out,size_t n) { (void)key;(void)out;(void)n;return -1; }
int persist_write_data(uint32_t key,const void *data,size_t n) { (void)key;(void)data;return (int)n; }
bool clock_is_24h_style(void) { return true; }
void kit_world_fill(LabData *d,int zone) { (void)d;(void)zone; }
Tuple *dict_find(DictionaryIterator *d,uint32_t key) {
  (void)d;
  if(key==MESSAGE_KEY_DARK_MODE && (present&1))return &tuples[0];
  if(key==MESSAGE_KEY_DARK_GRID && (present&2))return &tuples[1];
  return NULL;
}
static bool receive(unsigned mask,int mode,int grid,int type) {
  present=mask;values[0].int32=mode;values[1].int32=grid;
  for(int i=0;i<2;++i)tuples[i]=(Tuple){type,4,&values[i]};
  return kit_consumer_settings_receive(NULL);
}
int main(void) {
  LabData d={.weather_ok=true,.sunrise_min=449,.sunset_min=1083,.minute_of_day=600};
  kit_consumer_settings_init();
  assert(face_settings_get().appearance==FACE_LIGHT && !face_settings_get().white_grid);
  assert(!face_dark_mode(&d));
  // Exact legacy persistence layout and enum values retain both previous choices.
  for(int legacy=0;legacy<=1;++legacy) {
    saved[0]=legacy;sizes[0]=4;sizes[1]=0;
    kit_consumer_settings_init();
    assert(face_settings_get().appearance==legacy && !face_settings_get().white_grid);
    assert(face_dark_mode(&d)==(legacy==1));
  }
  assert(receive(3,FACE_AUTOMATIC,1,TUPLE_UINT));
  for(int minute=0;minute<1440;++minute) {
    d.minute_of_day=minute;
    assert(face_dark_mode(&d)==(minute<449 || minute>=1083));
  }
  d.weather_ok=false;assert(!face_dark_mode(&d));
  d.weather_ok=true;d.sunrise_min=d.sunset_min=-1;assert(!face_dark_mode(&d));
  d.sunrise_min=1083;d.sunset_min=449;assert(!face_dark_mode(&d));
  d.sunrise_min=449;d.sunset_min=1083;
  d.minute_of_day=-1;assert(!face_dark_mode(&d));
  d.minute_of_day=1440;assert(!face_dark_mode(&d));
  kit_consumer_settings_init();
  assert(face_settings_get().appearance==FACE_AUTOMATIC && face_settings_get().white_grid);
  int before=writes;
  assert(!receive(3,3,0,TUPLE_INT));
  assert(!receive(3,0,2,TUPLE_INT));
  assert(!receive(3,-1,0,TUPLE_INT));
  assert(!receive(3,0,-1,TUPLE_INT));
  assert(!receive(3,0,0,99));
  assert(!receive(0,0,0,TUPLE_INT));
  assert(writes==before && face_settings_get().appearance==FACE_AUTOMATIC && face_settings_get().white_grid);
  assert(receive(1,FACE_DARK,0,TUPLE_INT));
  assert(face_settings_get().white_grid);
  d.weather_ok=false;assert(face_dark_mode(&d));
  assert(receive(2,0,0,TUPLE_INT));assert(face_settings_get().appearance==FACE_DARK);
  before=writes;assert(!receive(3,FACE_DARK,0,TUPLE_INT));assert(writes==before);
  saved[0]=99;saved[1]=1;kit_consumer_settings_init();
  assert(face_settings_get().appearance==FACE_LIGHT && face_settings_get().white_grid);
  saved[0]=FACE_AUTOMATIC;sizes[0]=1;saved[1]=99;kit_consumer_settings_init();
  assert(face_settings_get().appearance==FACE_LIGHT && !face_settings_get().white_grid);
  puts("Mondrian appearance passed: light default, legacy records, full-day sun boundaries, fallback, persistence and atomic validation.");
}
