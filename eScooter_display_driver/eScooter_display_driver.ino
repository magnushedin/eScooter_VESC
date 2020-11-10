#include "NexButton.h"

NexButton b0 = NexButton(0, 2, "b0");

NexTouch *nex_listen_list[] =
{
  &b0,
  NULL
};

void buttonB0_Pop_Cbk(void *ptr)
{
  digitalWrite(13, LOW);
}

void buttonB0_Push_Cbk(void *ptr)
{
  digitalWrite(13, HIGH);
}

void setup() {
  // put your setup code here, to run once:
  nexInit();
  b0.attachPush(buttonB0_Push_Cbk, &b0);
  b0.attachPop(buttonB0_Pop_Cbk, &b0);
  digitalWrite(13, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  nexLoop(nex_listen_list);
}
