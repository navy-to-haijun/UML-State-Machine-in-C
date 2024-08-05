/**
 * \file
 * \brief hierarchical state machine
 * 
 */

#ifndef HSM_H
#define HSM_H

#ifdef HSM_CONFIG
#include "hsm_config.h"
#endif // HSM_CONFIG

/*
 *  --------------------- DEFINITION ---------------------
 */
/*是否开启分层状态机*/
#ifndef HIERARCHICAL_STATES
#define  HIERARCHICAL_STATES    0
#endif // HIERARCHICAL_STATES

/*是否开启状态机日志*/
#ifndef STATE_MACHINE_LOGGER
#define STATE_MACHINE_LOGGER     1 
#endif // STATE_MACHINE_LOGGER

/*是否开启变量长度数组*/
#ifndef HSM_USE_VARIABLE_LENGTH_ARRAY
#define HSM_USE_VARIABLE_LENGTH_ARRAY 0
#endif

/*
 *  --------------------- ENUMERATION ---------------------
 */

/*状态机返回值枚举*/
typedef enum
{
  EVENT_HANDLED,      // 事件处理成功
  EVENT_UN_HANDLED,   // 事件处理失败
  TRIGGERED_TO_SELF,  // 事件处理完状态回到自身
}state_machine_result_t;

/*
 *  --------------------- STRUCTURE ---------------------
 */

#if HIERARCHICAL_STATES
typedef struct hierarchical_state state_t;
#else
typedef struct finite_state state_t;
#endif // HIERARCHICAL_STATES

typedef struct state_machine_t state_machine_t;
typedef state_machine_result_t (*state_handler) (state_machine_t* const State);
typedef void (*state_machine_event_logger)(uint32_t state_machine, uint32_t state, uint32_t event);
typedef void (*state_machine_result_logger)(uint32_t state, state_machine_result_t result);

/*有限状态结构体*/
struct finite_state{
  state_handler Handler;       // 状态动作函数指针
  state_handler Entry;         // 状态进入动作函数指针
  state_handler Exit;         // 状态退出动作函数指针

#if STATE_MACHINE_LOGGER
  uint32_t Id;              //!< unique identifier of state within the single state machine
#endif
};

/*分层状态机结构体*/
struct hierarchical_state
{
  state_handler Handler;      // 状态动作函数指针
  state_handler Entry;        // 状态进入动作函数指针
  state_handler Exit;         // 状态退出动作函数指针

#if STATE_MACHINE_LOGGER
  uint32_t Id;              //!< unique identifier of state within the single state machine
#endif

  const state_t* const Parent;    // 父状态
  const state_t* const Node;      // 子状态
  uint32_t Level;                 // 该状态的层级（和顶层相比）
};

/*抽象出状态机结构体*/
struct state_machine_t
{
   uint32_t Event;          // 状态机待处理的事件
   const state_t* State;    // 状态机的状态
};

/*
 *  --------------------- EXPORTED FUNCTION ---------------------
 */

#ifdef __cplusplus
extern "C"  {
#endif // __cplusplus

extern state_machine_result_t dispatch_event(state_machine_t* const pState_Machine[],
                                            uint32_t quantity
#if STATE_MACHINE_LOGGER
                                            ,state_machine_event_logger event_logger
                                            ,state_machine_result_logger result_logger
#endif // STATE_MACHINE_LOGGER
                                            );

#if HIERARCHICAL_STATES
extern state_machine_result_t traverse_state(state_machine_t* const pState_Machine,
                                                       const state_t* pTarget_State);
#endif // HIERARCHICAL_STATES

extern state_machine_result_t switch_state(state_machine_t* const pState_Machine,
                                                    const state_t* const pTarget_State);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HSM_H
