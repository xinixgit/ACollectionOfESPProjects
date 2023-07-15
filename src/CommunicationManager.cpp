#include "CommunicationManager.h"

MessageTriggeredActionFn onOffAction(MessageTriggeredActionFn onAction, MessageTriggeredActionFn offAction)
{
  return [onAction, offAction](string payload)
  {
    if (strcmp(payload.c_str(), "on") == 0)
    {
      onAction(payload);
    }
    else if (strcmp(payload.c_str(), "off") == 0)
    {
      offAction(payload);
    }
  };
}