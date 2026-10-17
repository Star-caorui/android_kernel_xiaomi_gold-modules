/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM binder_prio

#if !defined(_TRACE_BINDER_PRIO_H) || defined(TRACE_HEADER_MULTI_INCLUDE)
#define _TRACE_BINDER_PRIO_H

#include <linux/tracepoint.h>

TRACE_EVENT(binder_prio_proc_transaction_finish,
	TP_PROTO(const char *msg),
	TP_ARGS(msg),
	TP_STRUCT__entry(
		__string(msg, msg)
	),
	TP_fast_assign(
		__assign_str(msg, msg);
	),
	TP_printk("%s", __get_str(msg))
);

#endif /* _TRACE_BINDER_PRIO_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE binder_prio
#include <trace/define_trace.h>
