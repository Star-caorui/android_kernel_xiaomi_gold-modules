/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Xiaomi's additions to the UFS host controller core: error statistics
 * for mi_memory, error reports to MIUI's event service, and what the CLD
 * and FFU features need from the core.
 */
#ifndef _UFSCTX_H
#define _UFSCTX_H

#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/blktrace_api.h>
#include <linux/blkdev.h>
#include <linux/bitfield.h>
#include <scsi/scsi_cmnd.h>
#include <ufs/ufshcd.h>

/* Host UIC error code PHY adapter layer */
enum ufshcd_ec_pa {
	UFS_EC_PA_LANE_0,
	UFS_EC_PA_LANE_1,
	UFS_EC_PA_LANE_2,
	UFS_EC_PA_LANE_3,
	UFS_EC_PA_LINE_RESET,
	UFS_EC_PA_MAX,
};

/* Host UIC error code data link layer */
enum ufshcd_ec_dl {
	UFS_EC_DL_NAC_RECEIVED,
	UFS_EC_DL_TCx_REPLAY_TIMER_EXPIRED,
	UFS_EC_DL_AFCx_REQUEST_TIMER_EXPIRED,
	UFS_EC_DL_FCx_PROTECT_TIMER_EXPIRED,
	UFS_EC_DL_CRC_ERROR,
	UFS_EC_DL_RX_BUFFER_OVERFLOW,
	UFS_EC_DL_MAX_FRAME_LENGTH_EXCEEDED,
	UFS_EC_DL_WRONG_SEQUENCE_NUMBER,
	UFS_EC_DL_AFC_FRAME_SYNTAX_ERROR,
	UFS_EC_DL_NAC_FRAME_SYNTAX_ERROR,
	UFS_EC_DL_EOF_SYNTAX_ERROR,
	UFS_EC_DL_FRAME_SYNTAX_ERROR,
	UFS_EC_DL_BAD_CTRL_SYMBOL_TYPE,
	UFS_EC_DL_PA_INIT_ERROR,
	UFS_EC_DL_PA_ERROR_IND_RECEIVED,
	UFS_EC_DL_MAX,
};

/* The core's own failures, after the events of enum ufs_event_type */
enum ufs_xiaomi_err_event {
	UFS_XM_EVT_WAIT_UIC_CMD_FAIL = 30,
	UFS_XM_EVT_WAIT_DEV_CMD_FAIL,
	UFS_XM_EVT_UIC_PWR_CTRL_FAIL,
	UFS_XM_EVT_HIBERN8_ENTER_FAIL,
	UFS_XM_EVT_HIBERN8_EXIT_FAIL,
	UFS_XM_EVT_CHANGE_PWR_MODE_FAIL,
	UFS_XM_EVT_ERR_HANDLER,
	UFS_XM_EVT_TRY_TO_ABORT_TASK,
};

struct ufs_uic_state {
	u32 pa_err_cnt;
	u32 pa_err[UFS_EC_PA_MAX];
	u32 dl_err_cnt;
	u32 dl_err[UFS_EC_DL_MAX];
	u32 dme_err_cnt;
};

struct ufs_err_state {
	u64 err_occurred;
	char err_reason[10][32];
};

struct ufs_xiaomi_ctx {
	struct ufs_hba *hba;
	struct ufs_uic_state uic_state;
	struct ufs_err_state err_state;
	/* completed once the logical units are added */
	struct completion ufslink_comp;
};

/* MIUI's event service (miev): the UFS error event and its key */
#define UFS_MIEVENT_ID		910013002
#define UFS_MIEVENT_KEY		"errInfo"

struct misight_mievent;
struct misight_mievent *cdev_tevent_alloc(unsigned int eventid);
int cdev_tevent_add_str(struct misight_mievent *event, const char *key,
			const char *value);
int cdev_tevent_write(struct misight_mievent *event);
void cdev_tevent_destroy(struct misight_mievent *event);

struct ufs_xiaomi_ctx *get_ufs_xiaomi(void);
void mievent_upload(int eventid, char *key, char *value);
void ufshcd_update_err_state(u32 id, u32 val);
void ufshcd_update_uic_error_cnt(u32 id, u32 val);

void ufshcd_scsi_block_requests(struct ufs_hba *hba);
void ufshcd_scsi_unblock_requests(struct ufs_hba *hba);
int ufshcd_wait_for_doorbell_clr(struct ufs_hba *hba, u64 wait_timeout_us);

int ufshcd_query_descriptor_retry_xm(struct ufs_hba *hba,
				     enum query_opcode opcode,
				     enum desc_idn idn, u8 index,
				     u8 selector,
				     u8 *desc_buf, int *buf_len);
int ufshcd_read_desc_param_sel(struct ufs_hba *hba,
			       enum desc_idn desc_id,
			       int desc_index,
			       u8 selector,
			       u8 param_offset,
			       u8 *param_read_buf,
			       u8 param_size);

void xm_scsi_register(struct ufs_hba *hba);
void xm_scsi_unregister(void);

#endif
