#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/backlight.h>
#include <linux/math64.h>
#include <linux/errno.h>

#define OPLUS_BL_MAX 8191U
#define OPLUS_BL_DEVICE "panel0-backlight"

static struct kobject *touchpanel_kobj;

static int match_name(struct device *dev, const void *data)
{
	return strcmp(dev_name(dev), (const char *)data) == 0;
}

/*
 * Return a backlight device with a held device reference.
 * The caller must put_device(&bd->dev) when finished.
 */
static struct backlight_device *find_panel_bl(void)
{
	struct backlight_device *any;
	struct device *dev;

	any = backlight_device_get_by_type(BACKLIGHT_RAW);
	if (!any)
		return NULL;

	dev = class_find_device(any->dev.class, NULL,
				OPLUS_BL_DEVICE, match_name);

	/* Release the reference acquired by backlight_device_get_by_type(). */
	put_device(&any->dev);

	if (!dev)
		return NULL;

	/*
	 * class_find_device() returns a referenced device.
	 * Keep that reference for the caller.
	 */
	return to_backlight_device(dev);
}

static ssize_t oplus_backlight_show(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    char *buf)
{
	struct backlight_device *bd;
	u64 value;
	unsigned int max_brightness;

	bd = find_panel_bl();
	if (!bd)
		return -ENODEV;

	max_brightness = bd->props.max_brightness;
	if (!max_brightness) {
		put_device(&bd->dev);
		return -EINVAL;
	}

	value = (u64)bd->props.brightness * OPLUS_BL_MAX;
	do_div(value, max_brightness);

	put_device(&bd->dev);

	return scnprintf(buf, PAGE_SIZE, "%llu\n",
			 (unsigned long long)value);
}

static ssize_t oplus_backlight_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct backlight_device *bd;
	unsigned int value;
	unsigned int max_brightness;
	u64 scaled;
	char input[32];
	size_t len = count;

	/* Parse the input before looking up the device. */
	if (!len || len >= sizeof(input))
		return -EINVAL;

	memcpy(input, buf, len);
	input[len] = '\0';

	/* Remove the sysfs newline. */
	if (len && input[len - 1] == '\n')
		input[--len] = '\0';

	/* Accept ColorOS values such as 8191.0. */
	if (len >= 2 && input[len - 2] == '.' &&
	    input[len - 1] == '0') {
		input[len - 2] = '\0';
	}

	if (kstrtouint(input, 10, &value)) {
		pr_err("oplus_backlight: invalid input\n");
		return -EINVAL;
	}

	if (value > OPLUS_BL_MAX)
		value = OPLUS_BL_MAX;

	bd = find_panel_bl();
	if (!bd)
		return -ENODEV;

	max_brightness = bd->props.max_brightness;
	if (!max_brightness) {
		put_device(&bd->dev);
		return -EINVAL;
	}

	/* Convert the ColorOS 0..8191 range to native brightness. */
	scaled = (u64)value * max_brightness;
	do_div(scaled, OPLUS_BL_MAX);

	if (value && !scaled)
		scaled = 1;

	pr_debug("oplus_backlight: requested=%u scaled=%llu max=%u\n",
		 value, (unsigned long long)scaled, max_brightness);

	backlight_device_set_brightness(bd, (unsigned long)scaled);

	put_device(&bd->dev);

	return count;
}

static struct kobj_attribute oplus_backlight_attr =
	__ATTR(oplus_backlight, 0664,
	       oplus_backlight_show, oplus_backlight_store);

static int __init oplus_backlight_node_init(void)
{
	int ret;

	touchpanel_kobj = kobject_create_and_add("touchpanel", NULL);
	if (!touchpanel_kobj)
		return -ENOMEM;

	ret = sysfs_create_file(touchpanel_kobj,
				&oplus_backlight_attr.attr);
	if (ret) {
		kobject_put(touchpanel_kobj);
		touchpanel_kobj = NULL;
		return ret;
	}

	pr_info("oplus_backlight: registered successfully\n");
	return 0;
}

device_initcall(oplus_backlight_node_init);
