#include <stdlib.h>
#include <mrubyc.h>
#include "../include/ws2812.h"

static void
c_ws2812_init(mrbc_vm *vm, mrbc_value *v, int argc)
{
  uint pin = (uint)GET_INT_ARG(1);
  uint pixel_size = (uint)GET_INT_ARG(2);

  ws2812_t *ws = MRBC_INSTANCE_DATA_PTR(v, ws2812_t);
  int ret = ws2812_init(ws, pin, pixel_size);
  if (ret < 0) {
    SET_INT_RETURN(-1);
    return;
  }

  SET_INT_RETURN(0);
}

static void
ws2812_destructor(mrbc_value *obj)
{
  ws2812_t *ws = MRBC_INSTANCE_DATA_PTR(obj, ws2812_t);
  if (ws != NULL) {
    ws2812_cleanup(ws);
  }
}

static void
c_ws2812_set_pixel_at_rgb(mrbc_vm *vm, mrbc_value *v, int argc)
{
  uint index = (uint)GET_INT_ARG(1);
  uint8_t r = (uint8_t)GET_INT_ARG(2);
  uint8_t g = (uint8_t)GET_INT_ARG(3);
  uint8_t b = (uint8_t)GET_INT_ARG(4);

  ws2812_t *ws = MRBC_INSTANCE_DATA_PTR(v, ws2812_t);
  ws2812_set_pixel_rgb(ws, index, r, g, b);
  SET_INT_RETURN(0);
}

static void
c_ws2812_clear(mrbc_vm *vm, mrbc_value *v, int argc)
{
  ws2812_t *ws = MRBC_INSTANCE_DATA_PTR(v, ws2812_t);
  ws2812_clear(ws);
  SET_INT_RETURN(0);
}

static void
c_ws2812_show(mrbc_vm *vm, mrbc_value *v, int argc)
{
  ws2812_t *ws = MRBC_INSTANCE_DATA_PTR(v, ws2812_t);
  ws2812_show(ws);
  SET_INT_RETURN(0);
}

void
mrbc_ws2812_init(mrbc_vm *vm)
{
  mrbc_class *mrbc_class_WS2812 = mrbc_define_class(vm, "WS2812", mrbc_class_object);
  mrbc_define_destructor(mrbc_class_WS2812, ws2812_destructor);
  
  mrbc_define_method(vm, mrbc_class_WS2812, "_init", c_ws2812_init);
  mrbc_define_method(vm, mrbc_class_WS2812, "set_pixel_at_rgb", c_ws2812_set_pixel_at_rgb);
  mrbc_define_method(vm, mrbc_class_WS2812, "clear", c_ws2812_clear);
  mrbc_define_method(vm, mrbc_class_WS2812, "show", c_ws2812_show);
}