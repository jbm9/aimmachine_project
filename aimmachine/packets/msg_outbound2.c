char packet[] = {
  0x2a, 0x02,
  0x11, 0x11,
  0x00, 0x95,

  0x00, 0x04,
  0x00, 0x06,
  0x00, 0x00,

  's','n','a','c',
  'm','y','c','o','o','k','i','e',

  0x00, 0x01,

  0x0b,
  's','c',    'r', 'e',   'e',  'n',  'n',  'a',  'm',  'e',  'x', 

  0x00, 0x02,
  0x00, 0x5b,
  0x05, 0x01,
  0x00, 0x01, 
  0x01, 0x01,
  0x01,
  0x00, 0x52,
  0x00, 0x00, 0x00, 0x00, 
  
'<','H','T','M','L','>','<','B','O','D','Y','>','<','F','O','N','T',' ','F','A','C','E','=','"','A','r','i','a','l','"',' ','S','I','Z','E','=','2','.','C','O','L','O','R','=','#','0','0','0','0','0','0','>','w','h','e','e','.','<','/','F','O','N','T','>','<','/','B','O','D','Y','>','<','/','H','T','M','L','>',

  0x00, 0x0d,
  0x00, 0x12,

  0x00, 0x01,
  0x00, 0x05,
  0x2b, 0x00, 0x00, 0x2b, 0x40,

  0x00, 0x81,
  0x00, 0x05, 
  0x2b, 0x00, 0x00, 0x14, 0x4f,
};
int packet_len = sizeof(packet);
