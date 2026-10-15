#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/backlight.h>
#include <linux/math64.h>

#define OPLUS_BL_MAX 8191 

static struct kobject *touchpanel_kobj;

static int match_name(struct device *dev, const void *data)
{
	return !strcmp(dev_name(dev), (const char *)data);
}

static struct backlight_device *find_panel_bl(void)
{
	struct backlight_device *any;
	struct device *dev;

	any = backlight_device_get_by_type(BACKLIGHT_RAW);
	if (!any)
		return NULL;
	dev = class_find_device(any->dev.class, NULL,
				"panel0-backlight", match_name);
	if (!dev)
		return NULL;
	put_device(dev);
	return to_backlight_device(dev);
}

static ssize_t oplus_backlight_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct backlight_device *bd = find_panel_bl();
	u64 v;

	if (!bd)
		return -ENODEV;
	v = (u64)bd->props.brightness * OPLUS_BL_MAX;
	do_div(v, bd->props.max_brightness ? bd->props.max_brightness : 1);
	return scnprintf(buf, PAGE_SIZE, "%llu\n", v);
}

static ssize_t oplus_backlight_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct backlight_device *bd = find_panel_bl();
	unsigned int val;
	u64 scaled;

	if (!bd)
		return -ENODEV;
	if (kstrtouint(buf, 10, &val))
		return -EINVAL;
	if (val > OPLUS_BL_MAX)
		val = OPLUS_BL_MAX;

	scaled = (u64)val * bd->props.max_brightness;
	do_div(scaled, OPLUS_BL_MAX);
	if (!scaled && val)
		scaled = 1;

	backlight_device_set_brightness(bd, (unsigned long)scaled);
	return count;
}

static struct kobj_attribute oplus_backlight_attr =
	__ATTR(oplus_backlight, 0664, oplus_backlight_show, oplus_backlight_store);

static int __init oplus_backlight_node_init(void)
{
	touchpanel_kobj = kobject_create_and_add("touchpanel", NULL);
	if (!touchpanel_kobj)
		return -ENOMEM;
	if (sysfs_create_file(touchpanel_kobj, &oplus_backlight_attr.attr)) {
		kobject_put(touchpanel_kobj);
		touchpanel_kobj = NULL;
	}
	return 0;
}
device_initcall(oplus_backlight_node_init);
