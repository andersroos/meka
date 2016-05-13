#pragma once

//
// Lib for showing errors in various outputs.
//

enum class error : uint8_t
{
   EVENT_QUEUE_FULL,
   EVENT_QUEUE_EMPTY,
};


// Only implementation is no implementation right now.
void show_error(error error)
{
}

   
