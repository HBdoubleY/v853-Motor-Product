#ifndef LIBBTSTACK_H
#define LIBBTSTACK_H

#include "bluetoothadapter.h"

BlueToothAdapter * init_bluetooth_stack(BluetoothNotify * notify);

void uninit_bluetooth_stack(BlueToothAdapter * adapter);


#endif // LIBBTSTACK_H
