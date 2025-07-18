class WS2812
  def initialize(pin, pixel_size)
    @pin = pin
    @pixel_size = pixel_size
    _init(pin, pixel_size)
  end

  attr_reader :pin, :pixel_size

  def set_pixel_at(index, rgb)
    r, g, b = rgb
    set_pixel_at_rgb(index, r, g, b)
  end
end