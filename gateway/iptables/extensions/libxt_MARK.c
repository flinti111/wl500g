/* Shared library add-on to iptables to add MARK target support. */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <xtables.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_MARK.h>

enum {
	F_MARK = 1 << 0,
};

static const struct option MARK_opts[] = {
	{ "set-mark", 1, NULL, '1' },
	{ "and-mark", 1, NULL, '2' },
	{ "or-mark", 1, NULL, '3' },
	{ .name = NULL }
};

static const struct option mark_tg_opts[] = {
	{.name = "set-xmark", .has_arg = true, .val = 'X'},
	{.name = "set-mark",  .has_arg = true, .val = '='},
	{.name = "and-mark",  .has_arg = true, .val = '&'},
	{.name = "or-mark",   .has_arg = true, .val = '|'},
	{.name = "xor-mark",  .has_arg = true, .val = '^'},
	{ .name = NULL }
};

static void mark_tg_help(void)
{
	printf(
"MARK target options:\n"
"  --set-xmark value[/mask]  Clear bits in mask and XOR value into nfmark\n"
"  --set-mark value[/mask]   Clear bits in mask and OR value into nfmark\n"
"  --and-mark bits           Binary AND the nfmark with bits\n"
"  --or-mark bits            Binary OR the nfmark with bits\n"
"  --xor-mask bits           Binary XOR the nfmark with bits\n"
"\n");
}

/* Function which parses command options; returns true if it
   ate an option */
static int mark_tg_parse(int c, char **argv, int invert, unsigned int *flags,
                         const void *entry, struct xt_entry_target **target)
{
	struct xt_mark_tginfo2 *info = (void *)(*target)->data;
	unsigned int value, mask = UINT32_MAX;
	char *end;

	switch (c) {
	case 'X': /* --set-xmark */
	case '=': /* --set-mark */
		xtables_param_act(XTF_ONE_ACTION, "MARK", *flags & F_MARK);
		xtables_param_act(XTF_NO_INVERT, "MARK", "--set-xmark/--set-mark", invert);
		if (!xtables_strtoui(optarg, &end, &value, 0, UINT32_MAX))
			xtables_param_act(XTF_BAD_VALUE, "MARK", "--set-xmark/--set-mark", optarg);
		if (*end == '/')
			if (!xtables_strtoui(end + 1, &end, &mask, 0, UINT32_MAX))
				xtables_param_act(XTF_BAD_VALUE, "MARK", "--set-xmark/--set-mark", optarg);
		if (*end != '\0')
			xtables_param_act(XTF_BAD_VALUE, "MARK", "--set-xmark/--set-mark", optarg);
		info->mark = value;
		info->mask = mask;

		if (c == '=')
			info->mask = value | mask;
		break;

	case '&': /* --and-mark */
		xtables_param_act(XTF_ONE_ACTION, "MARK", *flags & F_MARK);
		xtables_param_act(XTF_NO_INVERT, "MARK", "--and-mark", invert);
		if (!xtables_strtoui(optarg, NULL, &mask, 0, UINT32_MAX))
			xtables_param_act(XTF_BAD_VALUE, "MARK", "--and-mark", optarg);
		info->mark = 0;
		info->mask = ~mask;
		break;

	case '|': /* --or-mark */
		xtables_param_act(XTF_ONE_ACTION, "MARK", *flags & F_MARK);
		xtables_param_act(XTF_NO_INVERT, "MARK", "--or-mark", invert);
		if (!xtables_strtoui(optarg, NULL, &value, 0, UINT32_MAX))
			xtables_param_act(XTF_BAD_VALUE, "MARK", "--or-mark", optarg);
		info->mark = value;
		info->mask = value;
		break;

	case '^': /* --xor-mark */
		xtables_param_act(XTF_ONE_ACTION, "MARK", *flags & F_MARK);
		xtables_param_act(XTF_NO_INVERT, "MARK", "--xor-mark", invert);
		if (!xtables_strtoui(optarg, NULL, &value, 0, UINT32_MAX))
			xtables_param_act(XTF_BAD_VALUE, "MARK", "--xor-mark", optarg);
		info->mark = value;
		info->mask = 0;
		break;

	default:
		return false;
	}

	*flags |= F_MARK;
	return true;
}

static void mark_tg_check(unsigned int flags)
{
	if (flags == 0)
		xtables_error(PARAMETER_PROBLEM, "MARK: One of the --set-xmark, "
		           "--{and,or,xor,set}-mark options is required");
}

static void mark_tg_print(const void *ip, const struct xt_entry_target *target,
                          int numeric)
{
	const struct xt_mark_tginfo2 *info = (const void *)target->data;

	if (info->mark == 0)
		printf("MARK and 0x%x ", (unsigned int)(u_int32_t)~info->mask);
	else if (info->mark == info->mask)
		printf("MARK or 0x%x ", info->mark);
	else if (info->mask == 0)
		printf("MARK xor 0x%x ", info->mark);
	else
		printf("MARK xset 0x%x/0x%x ", info->mark, info->mask);
}

static void mark_tg_save(const void *ip, const struct xt_entry_target *target)
{
	const struct xt_mark_tginfo2 *info = (const void *)target->data;

	printf("--set-xmark 0x%x/0x%x ", info->mark, info->mask);
}

static struct xtables_target mark_tg_reg_v2 = {
	.version       = XTABLES_VERSION,
	.name          = "MARK",
	.revision      = 2,
	.family        = AF_UNSPEC,
	.size          = XT_ALIGN(sizeof(struct xt_mark_tginfo2)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_mark_tginfo2)),
	.help          = mark_tg_help,
	.parse         = mark_tg_parse,
	.final_check   = mark_tg_check,
	.print         = mark_tg_print,
	.save          = mark_tg_save,
	.extra_opts    = mark_tg_opts,
};

void _init(void)
{
	xtables_register_target(&mark_tg_reg_v2);
}
