/**
 * \file
 * \brief hierarchical state machine
 */

/*
 *  --------------------- INCLUDE FILES ---------------------
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "hsm.h"

/*
 *  --------------------- DEFINITION ---------------------
 */

#define EXECUTE_HANDLER(handler, triggerd, state_machine)       \
do{                                                             \
  if(handler != NULL)                                           \
  {                                                             \
    state_machine_result_t result = handler(state_machine);     \
    switch(result)                                              \
    {                                                           \
    case TRIGGERED_TO_SELF:                                     \
      triggerd = true;                                          \
    case EVENT_HANDLED:                                         \
      break;                                                    \
                                                                \
    default:                                                    \
      return result;                                            \
    }                                                           \
  }                                                             \
} while(0)

/*
 *  --------------------- FUNCTION BODY ---------------------
 */

/** \brief 将事件派发到所有状态机
 *
 * \param pState_Machine[] 状态机数组
 * \param quantity        状态机数量
 * \return                状态机的处理结果
 *
 */
state_machine_result_t dispatch_event(state_machine_t* const pState_Machine[]
                                      ,uint32_t quantity
#if STATE_MACHINE_LOGGER
                                      ,state_machine_event_logger event_logger
                                      ,state_machine_result_logger result_logger
#endif // STATE_MACHINE_LOGGER
                                      )
{
  state_machine_result_t result;

  // 遍历所以状态机，查看是否有需要分配的事件
  for(uint32_t index = 0; index < quantity;)
  {
    if(pState_Machine[index]->Event == 0)
    {
      index++;
      continue;
    }

    const state_t* pState = pState_Machine[index]->State;
    do
    {
#if STATE_MACHINE_LOGGER
      event_logger(index, pState->Id, pState_Machine[index]->Event);
#endif // STATE_MACHINE_LOGGER
        // Call the state handler.
      result = pState->Handler(pState_Machine[index]);
#if STATE_MACHINE_LOGGER
      result_logger(pState_Machine[index]->State->Id, result);
#endif // STATE_MACHINE_LOGGER

      switch(result)
      {
      // 事件处理成功，，清除事件
      case EVENT_HANDLED:
        pState_Machine[index]->Event = 0;
        break;
      // 待定
      case TRIGGERED_TO_SELF:
        index = 0;  // Restart the event dispatcher from the first state machine.
        break;

    #if HIERARCHICAL_STATES
    // State handler could not handled the event.
    // Traverse to its parent state and dispatch event to parent state handler.
      case EVENT_UN_HANDLED:

        do
        {
          // check if state has parent state.
          if(pState->Parent == NULL)   // Is Node reached top
          {
            // This is a fatal error. terminate state machine.
            return EVENT_UN_HANDLED;
          }

          pState = pState->Parent;        // traverse to parent state
        }while(pState->Handler == NULL);   // repeat again if parent state doesn't have handler
        continue;
    #endif // HIERARCHICAL_STATES

      // Either state handler could not handle the event or it has returned
      // the unknown return code. Terminate the state machine.
      default:
        return result;
      }
      break;

    }while(1);
  }
  return EVENT_HANDLED;
}

/** \brief  切换状态
 *
 * \param pState_Machine  状态机
 * \param pTarget_State   需要转换的状态
 * \return                切换结果
 *
 */
extern state_machine_result_t switch_state(state_machine_t* const pState_Machine,
                                           const state_t* const pTarget_State)
{
  const state_t* const pSource_State = pState_Machine->State;
  bool triggered_to_self = false;
  pState_Machine->State = pTarget_State;    // Save the target node

  // 执行退出动作（上一个状态）
    EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
  // 执行进入状态（目标状态）
    EXECUTE_HANDLER(pTarget_State->Entry, triggered_to_self, pState_Machine);

  if(triggered_to_self == true)
  {
    return TRIGGERED_TO_SELF;
  }

  return EVENT_HANDLED;
}

#if HIERARCHICAL_STATES
/** \brief Traverse to target state. It calls exit functions before leaving
      the source state & calls entry function before entering the target state.
 *
 * \param pState_Machine state_machine_t* const   pointer to state machine
 * \param pTarget_State const state_t*            Target state to traverse
 * \return state_machine_result_t                 Result of state traversal
 *
 */
state_machine_result_t traverse_state(state_machine_t* const pState_Machine,
                                              const state_t* pTarget_State)
{
  const state_t *pSource_State = pState_Machine->State;
  bool triggered_to_self = false;
  pState_Machine->State = pTarget_State;    // Save the target node

#if (HSM_USE_VARIABLE_LENGTH_ARRAY == 1)
  const state_t *pTarget_Path[pTarget_State->Level];  // Array to store the target node path
#else
  #if  (!defined(MAX_HIERARCHICAL_LEVEL) || (MAX_HIERARCHICAL_LEVEL == 0))
  #error "MAX_HIERARCHICAL_LEVEL is undefined."\
         "Define the maximum hierarchical level of the state machine or \
          use variable length array by setting HSM_USE_VARIABLE_LENGTH_ARRAY to 1"
  #endif

  const state_t* pTarget_Path[MAX_HIERARCHICAL_LEVEL];     // Array to store the target node path
#endif

  uint32_t index = 0;

  // make the source state & target state at the same hierarchy level.

  // Is source hierarchy level is less than target hierarchy level?
  if(pSource_State->Level > pTarget_State->Level)
  {
    // Traverse the source state to upward,
    // till it matches with target state hierarchy level.
    while(pSource_State->Level > pTarget_State->Level)
    {
      EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
      pSource_State = pSource_State->Parent;
    }
  }
  // Is Source hierarchy level greater than target level?
  else if(pSource_State->Level < pTarget_State->Level)
  {
    // Traverse the target state to upward,
    // Till it matches with source state hierarchy level.
    while(pSource_State->Level < pTarget_State->Level)
    {
      pTarget_Path[index++] = pTarget_State;  // Store the target node path.
      pTarget_State = pTarget_State->Parent;
    }
  }

  // Now Source & Target are at same hierarchy level.
  // Traverse the source & target state to upward, till we find their common parent.
  while(pSource_State->Parent != pTarget_State->Parent)
  {
    EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
    pSource_State = pSource_State->Parent;  // Move source state to upward state.

    pTarget_Path[index++] = pTarget_State;  // Store the target node path.
    pTarget_State = pTarget_State->Parent;    // Move the target state to upward state.
  }

  // Call Exit function before leaving the Source state.
    EXECUTE_HANDLER(pSource_State->Exit, triggered_to_self, pState_Machine);
  // Call entry function before entering the target state.
    EXECUTE_HANDLER(pTarget_State->Entry, triggered_to_self, pState_Machine);

    // Now traverse down to the target node & call their entry functions.
    while(index)
    {
      index--;
      EXECUTE_HANDLER(pTarget_Path[index]->Entry, triggered_to_self, pState_Machine);
    }

  if(triggered_to_self == true)
  {
    return TRIGGERED_TO_SELF;
  }
  return EVENT_HANDLED;
}
#endif // HIERARCHICAL_STATES

