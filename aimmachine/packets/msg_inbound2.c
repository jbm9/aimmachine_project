char packet[] = {
  0x2a, 0x02,
  0x11, 0x11,
  0x00, 0x81,
  0x00, 0x04, 0x00, 0x07,
  0x00, 0x00,
  's','n','a','c',
  'm','y','c','o','o','k','i','e',

  0x00, 0x01,
  0x0a, 
  'o','e','p','r','o','x','t','e','s','t',
  0x00, 0x00,

  0x00, 0x05,

  0x00, 0x01,
  0x00, 0x02,
  0x00, 0x11,

  0x00, 0x05,
  0x00, 0x04,
  0x47, 0x37, 0x69, 0x2a,

  0x00, 0x1d,
  0x00, 0x12,
  0x00, 0x01, 0x00, 0x05, 0x02, 0x01, 0xd2, 0x04,
  0x72, 0x00, 0x81, 0x00, 0x05, 0x2b, 0x00, 0x00,
  0x14, 0x4f,

  0x00, 0x0f,
  0x00, 0x04,
  0x00, 0x00, 0xff, 0x35,

  0x00, 0x03,
  0x00, 0x04,
  0x47, 0x3f, 0xf6, 0x50,

  0x00, 0x02,
  0x00, 0x15,

  0x05, 0x01,
  0x00, 0x04,
  0x01, 0x01, 0x01, 0x02,
  0x01, 0x01,
  0x00, 0x09,
  0x00, 0x00,
  0x00, 0x00,
  'a','b','c','d','e',

  0x00, 0x0b,
  0x00, 0x00,

  0x00, 0x16,
  0x00, 0x04,
  0x47, 0x40, 0xf5, 0x85,

  0x00, 0x13,
  0x00, 0x01,
  0x09,
};
int packet_len = sizeof(packet);
