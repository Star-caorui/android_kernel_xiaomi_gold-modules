/*
 * miufs_cld1.c
 *
 *  Created on: 2020-10-20
 *      Author: shane
 */

#include "ufscld.h"
#include "ufscld_sysfs.h"

struct ufscld_dev *cld;

struct standard_inquiry {
	uint8_t vendor_id[8];
	uint8_t product_id[16];
	uint8_t product_rev[4];
};

#define CLD_DEBUG(cld, msg, args...)					\
	do { if (cld->cld_debug)					\
		pr_err("%40s:%3d [%01d%02d%02d] " msg "\n",		\
		       __func__, __LINE__,				\
			   cld->cld_trigger,				\
		       atomic_read(&cld->hba->dev->power.usage_count),\
			   cld->hba->clk_gating.active_reqs, ##args);	\
	} while (0)

struct vendor_ops cld_ops_arry[] = {
		{"SAMSUNG", AUTO_HIBERN8_ENABLE, &samsung_cld_ops},
		{"SKhynix", AUTO_HIBERN8_ENABLE, &hynix_cld_ops},
		{"KIOXIA", AUTO_HIBERN8_DISABLE, &kioxia_cld_ops},
		{"MICRON", AUTO_HIBERN8_ENABLE, &micron_cld_ops},
};

int ufscld_init_ops(struct ufscld_dev *cld)
{
	struct standard_inquiry stdinq = {};
	int ret = -1;
	int i = 0;
	struct ufs_hba *hba = cld->hba;

	memcpy(&stdinq, hba->ufs_device_wlun->inquiry + 8, sizeof(stdinq));

	for (i = 0; i < sizeof(cld_ops_arry)/sizeof(cld_ops_arry[0]); i++) {
		if (strncmp((char *)stdinq.vendor_id, (char *)cld_ops_arry[i].vendor_id, strlen((char *)cld_ops_arry[i].vendor_id)) == 0) {
			cld->cld_ops = cld_ops_arry[i].cld_ops;
			cld->vendor_ops = &cld_ops_arry[i];
			return 0;
		}
	}
	return ret;
}

int ufscld_is_not_present(struct ufscld_dev *cld)
{
	enum UFSCLD_STATE cur_state = ufscld_get_state(cld);

	if (cur_state != CLD_PRESENT) {
		INFO_MSG("cld_state != CLD_PRESENT (%d)", cur_state);
		return -ENODEV;
	}
	return 0;
}

int ufscld_get_state(struct ufscld_dev *cld)
{
	if (!cld) {
		INFO_MSG("cld is not init, please check.");
		return CLD_NEED_INIT;
	}

	return atomic_read(&cld->cld_state);
}

void ufscld_set_state(struct ufscld_dev *cld, int state)
{
	atomic_set(&cld->cld_state, state);
}


/*
 * Lock status: cld_sysfs lock was held when called.
 */
void ufscld_auto_hibern8_enable(struct ufscld_dev *cld,
				       unsigned int val)
{
	struct ufs_hba *hba = cld->hba;
	unsigned long flags;
	u32 reg;

	val = !!val;

	/* Update auto hibern8 timer value if supported */
	if (!ufshcd_is_auto_hibern8_supported(hba))
		return;

	pm_runtime_get_sync(&hba->ufs_device_wlun->sdev_gendev);
	ufshcd_hold(hba);

	ufshcd_scsi_block_requests(hba);
	/* wait for all the outstanding requests to finish */
	ufshcd_wait_for_doorbell_clr(hba, U64_MAX);

	spin_lock_irqsave(hba->host->host_lock, flags);

	reg = ufshcd_readl(hba, REG_AUTO_HIBERNATE_IDLE_TIMER);
	INFO_MSG("ahit-reg 0x%X", reg);

	/* the timer reads 0 while auto hibern8 is off */
	if (val ^ (reg != 0)) {
		if (val) {
			/*
			 * turn it back on with the timer saved when it was
			 * turned off
			 */
			hba->ahit = cld->ahit;
		} else {
			/* save the timer to turn it back on later */
			cld->ahit = reg;
			hba->ahit = 0;
		}

		ufshcd_writel(hba, hba->ahit, REG_AUTO_HIBERNATE_IDLE_TIMER);
		/* Make sure the timer gets applied before further operations */
		mb();

		INFO_MSG("[Before] is_auto_enabled %d", cld->is_auto_enabled);
		cld->is_auto_enabled = val;

		reg = ufshcd_readl(hba, REG_AUTO_HIBERNATE_IDLE_TIMER);
		INFO_MSG("[After] is_auto_enabled %d ahit-reg 0x%X",
			 cld->is_auto_enabled, reg);
	} else {
		INFO_MSG("is_auto_enabled %d. so it does not changed",
			 cld->is_auto_enabled);
	}

	spin_unlock_irqrestore(hba->host->host_lock, flags);
	ufshcd_scsi_unblock_requests(hba);
	ufshcd_release(hba);
	pm_runtime_put_sync(&cld->hba->ufs_device_wlun->sdev_gendev);
}

void ufscld_block_enter_suspend(struct ufscld_dev *cld)
{
	struct ufs_hba *hba = cld->hba;
	unsigned long flags;

	if (unlikely(cld->block_suspend))
		return;

	cld->block_suspend = true;

	pm_runtime_get_sync(&hba->ufs_device_wlun->sdev_gendev);
	ufshcd_hold(hba);

	spin_lock_irqsave(hba->host->host_lock, flags);
	CLD_DEBUG(cld,
		  "dev->power.usage_count %d hba->clk_gating.active_reqs %d",
		  atomic_read(&hba->dev->power.usage_count),
		  hba->clk_gating.active_reqs);
	spin_unlock_irqrestore(hba->host->host_lock, flags);
}

void ufscld_allow_enter_suspend(struct ufscld_dev *cld)
{
	struct ufs_hba *hba = cld->hba;
	unsigned long flags;

	if (unlikely(!cld->block_suspend))
		return;

	cld->block_suspend = false;

	ufshcd_release(hba);
	pm_runtime_put_sync(&cld->hba->ufs_device_wlun->sdev_gendev);

	spin_lock_irqsave(hba->host->host_lock, flags);
	CLD_DEBUG(cld,
		  "dev->power.usage_count %d hba->clk_gating.active_reqs %d",
		  atomic_read(&hba->dev->power.usage_count),
		  hba->clk_gating.active_reqs);
	spin_unlock_irqrestore(hba->host->host_lock, flags);
}

int ufscld_get_frag_level(struct ufscld_dev *cld, int *frag_level)
{

	int ret;

	ret = cld->cld_ops->cld_get_frag_level(cld, frag_level);

	return ret;
}

int ufscld_get_operation_status(struct ufscld_dev *cld, int *op_status)
{

	int ret;

	ret = cld->cld_ops->cld_operation_status(cld, op_status);

	return ret;
}

static int ufscld_issue_enable(struct ufscld_dev *cld)
{
	if (cld->cld_ops->cld_set_trigger(cld, 1))
		return -EINVAL;
	return 0;
}


static int ufscld_issue_disable(struct ufscld_dev *cld)
{
	int frag_level;

	if (cld->cld_ops->cld_set_trigger(cld, 0))
		return -EINVAL;
	if (cld->cld_ops->cld_get_frag_level(cld, &frag_level))
		return -EINVAL;

	CLD_DEBUG(cld, "Frag_lv %d \n", frag_level);

	return 0;
}

static void trigger_uevent(struct ufscld_dev *cld, int trigger)
{
	char *cld_trigger_on[2]  = { "CLD_TRIGGER=ON", NULL };
	char *cld_trigger_off[2] = { "CLD_TRIGGER=OFF", NULL };

	if (trigger) {
		kobject_uevent_env(&cld->hba->dev->kobj, KOBJ_CHANGE, cld_trigger_on);
		INFO_MSG("UFSCLD: sent uevent %s\n", cld_trigger_on[0]);
	} else {
		kobject_uevent_env(&cld->hba->dev->kobj, KOBJ_CHANGE, cld_trigger_off);
		INFO_MSG("UFSCLD: sent uevent %s\n", cld_trigger_off[0]);
	}
}

/*
 * Keep the device resumed while the defrag is driven; fails when the CLD is
 * not usable, or the device did not come back from a system suspend.
 */
int ufscld_hold_runtime_pm(struct ufscld_dev *cld)
{
	struct ufs_hba *hba = cld->hba;

	/*
	 * Suspended along with the device: resume it, which brings the CLD
	 * back through ufscld_resume().
	 */
	if (ufscld_get_state(cld) == CLD_SUSPEND) {
		pm_runtime_get_sync(&hba->ufs_device_wlun->sdev_gendev);

		if (ufscld_get_state(cld) != CLD_PRESENT ||
		    hba->curr_dev_pwr_mode != UFS_ACTIVE_PWR_MODE ||
		    hba->uic_link_state != UIC_LINK_ACTIVE_STATE) {
			INFO_MSG("RPM resume failed. Maybe it was SPM suspend");
			INFO_MSG("UFS state (POWER = %d LINK = %d)",
				 hba->curr_dev_pwr_mode, hba->uic_link_state);
			ufscld_release_runtime_pm(cld);
			return -ENODEV;
		}
		return 0;
	}

	if (ufscld_is_not_present(cld))
		return -ENODEV;

	pm_runtime_get_sync(&hba->ufs_device_wlun->sdev_gendev);

	return 0;
}

void ufscld_release_runtime_pm(struct ufscld_dev *cld)
{
	pm_runtime_put_sync(&cld->hba->ufs_device_wlun->sdev_gendev);
}

/*
 * Lock status: cld_sysfs lock was held when called.
 */
int ufscld_trigger_on(struct ufscld_dev *cld)
{
	int ret;

	if (unlikely(cld->cld_trigger))
		return 0;

	ret = ufscld_hold_runtime_pm(cld);
	if (ret)
		return ret;

	cld->cld_trigger = true;

	ufscld_block_enter_suspend(cld);

	if (cld->vendor_ops->auto_hibern8_enable) {
		ufscld_auto_hibern8_enable(cld, 0);
	}

	/* the vendor ops time their own steps from start_time_stamp */
	cld->start_time_stamp = ktime_to_ms(ktime_get());
	/* the worker turns the defrag off after it ran too long */
	cld->trigger_time_stamp = ktime_to_ms(ktime_get());
	CLD_DEBUG(cld, "at %lld trigger 0 -> 1", cld->trigger_time_stamp);

	schedule_delayed_work(&cld->cld_trigger_work, 0);

	ufscld_release_runtime_pm(cld);

	return 0;
}

/*
 * Lock status: cld_sysfs lock was held when called.
 */
int ufscld_trigger_off(struct ufscld_dev *cld)
{
	int ret;

	if (unlikely(!cld->cld_trigger))
		return 0;

	ret = ufscld_hold_runtime_pm(cld);
	if (ret)
		return ret;

	cld->cld_trigger = false;
	CLD_DEBUG(cld, "cld_trigger 1 -> 0");

	ufscld_issue_disable(cld);

	if (cld->vendor_ops->auto_hibern8_enable) {
		ufscld_auto_hibern8_enable(cld, 1);
	}

	ufscld_allow_enter_suspend(cld);

	trigger_uevent(cld, 0);

	ufscld_release_runtime_pm(cld);

	return 0;
}

static void ufscld_trigger_work_fn(struct work_struct *dwork)
{
	struct ufscld_dev *cld;
	int ret;
	int op_status = 0;
	s64 duration;

	cld = container_of(dwork, struct ufscld_dev, cld_trigger_work.work);

	if (ufscld_hold_runtime_pm(cld))
		goto release;

	/*
	 * Block the suspend again if it was let go meanwhile.
	 */
	if (!cld->block_suspend) {
		ufscld_block_enter_suspend(cld);
		if (cld->vendor_ops->auto_hibern8_enable)
			ufscld_auto_hibern8_enable(cld, 0);
	}

	ufscld_get_operation_status(cld, &op_status);

	CLD_DEBUG(cld, "cld status %d\n", op_status);

	ret = ufscld_issue_enable(cld);

	if (ret) {
		ERR_MSG("ufscld_issue_enable failed ret=%d\n", ret);
	}

	mutex_lock(&cld->sysfs_lock);
	if (!cld->cld_trigger) { // if  host manual stop cld
		CLD_DEBUG(cld, "cld_trigger == false, return");
		mutex_unlock(&cld->sysfs_lock);
		return;
	}

	duration = ktime_to_ms(ktime_get()) - cld->trigger_time_stamp;
	if (CLD_STATUS_IDEL == op_status || duration >= CLD_TRIGGER_MAX_DURATION_MS) {
		WARN_MSG("CLD off with status (%d), duration (%lld ms)", op_status, duration);
		ufscld_trigger_off(cld);
		mutex_unlock(&cld->sysfs_lock);
		return;
	} else if (CLD_STATUS_PROGRESSING == op_status) {
		CLD_DEBUG(cld, "cld_REQUIRED, so sched (%d ms)",
				cld->cld_trigger_delay);
	} else {
		CLD_DEBUG(cld, "issue_cld ERR(%X), so resched for retry",
			  ret);
	}

	mutex_unlock(&cld->sysfs_lock);

	schedule_delayed_work(&cld->cld_trigger_work,
			      msecs_to_jiffies(cld->cld_trigger_delay));

release:
	ufscld_release_runtime_pm(cld);

	CLD_DEBUG(cld, "end cld_trigger_work_fn");
}

void ufscld_suspend(struct ufscld_dev *cld)
{
	if (!cld)
		return;

	if (cld->cld_trigger)
		ERR_MSG("cld_trigger was set to block the suspend. so weird");

	ufscld_set_state(cld, CLD_SUSPEND);
	cancel_delayed_work_sync(&cld->cld_trigger_work);
}

void ufscld_resume(struct ufscld_dev *cld)
{
	if (!cld)
		return;

	if (unlikely(cld->cld_trigger))
		ERR_MSG("cld_trigger need to off");

	ufscld_set_state(cld, CLD_PRESENT);
	if (cld->cld_trigger) {
		CLD_DEBUG(cld, "CLD trigger = %d, resume cld_trigger_work", cld->cld_trigger);
		schedule_delayed_work(&cld->cld_trigger_work, 0);
	}
}

/*
 * this function is called in irq context.
 * so cancel_delayed_work_sync() do not use due to waiting.
 */
void ufscld_on_idle(struct ufs_hba *hba)
{

	if (!cld->cld_trigger)
		return;// cld already done or not triggered.

	if (ufscld_get_state(cld) != CLD_PRESENT || hba->outstanding_reqs) {
		return; // HID is not avalible or this is not the last cmd.
	}

	if (!cld->cld_ops) // cld no supported
		return;

	/*
	 * When cld_trigger_work will be scheduled,
	 * check cld_trigger under sysfs_lock.
	 */

	if (delayed_work_pending(&cld->cld_trigger_work))
		cancel_delayed_work(&cld->cld_trigger_work);

	schedule_delayed_work(&cld->cld_trigger_work, msecs_to_jiffies(1000));// if there is no new cmd in 1S.
}

void ufscld_init(struct ufs_hba *hba)
{
	int ret = 0;

	cld = kzalloc(sizeof(struct ufscld_dev), GFP_KERNEL);

	cld->hba = hba;

	cld->cld_trigger = false;

	ret = ufscld_init_ops(cld);
	if (ret) {
		ERR_MSG("CLD get cld ops fail.\n");
		return;
	} else {
		INFO_MSG("CLD init ops succeed\n");
	}

	cld->cld_trigger_delay = CLD_TRIGGER_WORKER_DELAY_MS_DEFAULT;
	INIT_DELAYED_WORK(&cld->cld_trigger_work, ufscld_trigger_work_fn);

	cld->cld_debug = false;
	cld->block_suspend = false;
	cld->start_time_stamp = 0;
	cld->trigger_time_stamp = 0;

	/* the auto hibern8 timer, to restore after a defrag */
	cld->ahit = hba->ahit;

	/* If HCI supports auto hibern8, UFS Driver use it default */
	if (ufshcd_is_auto_hibern8_supported(cld->hba))
		cld->is_auto_enabled = true;
	else
		cld->is_auto_enabled = false;

	ret = ufscld_create_sysfs(cld);
	if (ret) {
		ERR_MSG("ufscld sysfs create failed.\n");
		ufscld_set_state(cld, CLD_FAILED);
		return;
	}

	INFO_MSG("UFS cld create sysfs finished.\n");

	ufscld_set_state(cld, CLD_PRESENT);

}

void ufscld_remove(struct ufs_hba *hba)
{
	ufscld_set_state(cld, CLD_FAILED);

	cancel_delayed_work_sync(&cld->cld_trigger_work);

	/* under the lock the sysfs entries take */
	mutex_lock(&cld->sysfs_lock);
	ufscld_allow_enter_suspend(cld);
	ufscld_trigger_off(cld);
	ufscld_remove_sysfs(cld);
	mutex_unlock(&cld->sysfs_lock);

	INFO_MSG("CLD released\n");
}


void ufscld_reset_host(struct ufs_hba *hba)
{
	if (!hba)
		return;
	ufscld_set_state(cld, CLD_RESET);
	cancel_delayed_work_sync(&cld->cld_trigger_work);
}

void ufscld_reset(struct ufs_hba *hba)
{

	ufscld_set_state(cld, CLD_PRESENT);

	/*
	 * cld_trigger will be checked under sysfs_lock in worker.
	 */
	if (cld->cld_trigger)
		schedule_delayed_work(&cld->cld_trigger_work, 0);

	INFO_MSG("reset completed.");
}
