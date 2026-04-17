#pragma once
/*
Purpose: V0 spike — hand-coded layout JSON embedded in flash.
Isolates the render-budget question from LittleFS I/O.
Three widgets: 1× logo_stub, 1× text, 1× clock. Targets 192x48 canvas.
*/

namespace v0spike {

// Widget schema (see CustomLayoutMode.cpp parser):
//   t = type string ("text" | "clock" | "logo_stub")
//   x,y,w,h = pixel bounds on 192x48 canvas
//   s = static text (TEXT only, <=31 chars)
//   r,g,b = color components (0-255)
//
// Layout sketch (192 wide x 48 tall):
//   [LOGO 32x32] [FLIGHTWALL V0 SPIKE]
//                [HH:MM clock]
static const char kLayoutJson[] =
    "{"
      "\"v\":1,"
      "\"w\":192,"
      "\"h\":48,"
      "\"widgets\":["
        "{\"id\":\"w1\",\"t\":\"logo_stub\",\"x\":2,\"y\":16,\"w\":16,\"h\":16,"
          "\"r\":255,\"g\":170,\"b\":0},"
        "{\"id\":\"w2\",\"t\":\"text\",\"x\":40,\"y\":10,\"w\":150,\"h\":8,"
          "\"s\":\"FLIGHTWALL V0\",\"r\":255,\"g\":255,\"b\":255},"
        "{\"id\":\"w3\",\"t\":\"clock\",\"x\":40,\"y\":24,\"w\":80,\"h\":12,"
          "\"r\":0,\"g\":255,\"b\":200}"
      "]"
    "}";

} // namespace v0spike
