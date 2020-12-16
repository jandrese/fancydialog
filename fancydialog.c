#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>

static struct option getopt_long_options[] =
{
	{"help",		no_argument,		0,	0},
	{"text",		required_argument,	0,	0},
	{"show-header",		no_argument,		0,	0},
	{"separator",		required_argument,	0,	0},
	{"add-label",		required_argument,	0,	0},
	{"add-separator",	no_argument,		0,	0},
	{"add-image",		required_argument,	0,	0},
	{"add-entry",		required_argument,	0,	0},
	{"entry-default",	required_argument,	0,	0},
	{"add-password",	required_argument,	0,	0},
	{"password-default",	required_argument,	0,	0},
	{"add-calendar",	required_argument,	0,	0},
	{"add-checkbox",	required_argument,	0,	0},
	{"checked",		no_argument,		0,	0},
	{"add-combo",		required_argument,	0,	0},
	{"combo-values",	required_argument,	0,	0},
	{"combo-default",	required_argument,	0,	0},
	{"add-file-selector",	required_argument,	0,	0},
	{"file-default",	required_argument,	0,	0},
	{"add-slider",		required_argument,	0,	0},
	{"slider-default",	required_argument,	0,	0},
	{"add-font",		required_argument,	0,	0},
	{"add-color",		required_argument,	0,	0},
	{"add-application",	required_argument,	0,	0},
	{"add-page-setup",	required_argument,	0,	0},
	{"add-print",		required_argument,	0,	0},
	{0,			0,			0,	0},
};


typedef struct
{
	int argc;
	char** argv;
	const char* previous_option;
	char* previous_optarg;
	int previous_hasarg;
	int option_queued;
} option_meta;

int get_next_opt(option_meta* meta, const char** option, char** value)
{
	int optval;
	int index;
	
	if ( meta->option_queued == 1 )
	{
		*option = meta->previous_option;
		*value = meta->previous_optarg;
		meta->option_queued = 0;
		return 0;
	}

	optval = getopt_long(meta->argc, meta->argv, "",
			getopt_long_options, &index);

	if ( optval < 0 )
		return optval;

	/* User made an error in the arguments */
	if ( optval == '?' )
	{
		exit(-1);
	}

	if ( optval > 0 )
	{
		fprintf(stderr, "getopt_long returned an unexpected value %d.  Logic bug in program.\n", optval);
	       return -1;
	}	       

	meta->previous_option = getopt_long_options[index].name;
	meta->previous_optarg = optarg;

	*option = getopt_long_options[index].name;
	*value = optarg;

	return 0;
}

int get_optional_default(option_meta* meta, const char* fieldname, char** value)
{
	const char* name;
	*value = NULL;

	if ( get_next_opt(meta, &name, value) == 0 )
	{
		if ( strcasecmp(name, fieldname) != 0 )
		{
			*value = NULL;
			meta->option_queued = 1;
		}
	}
	return 0;
}

int init_option_meta(option_meta* meta, int argc, char** argv)
{
	memset(meta, 0, sizeof(option_meta));
	meta->argc = argc;
	meta->argv = argv;
	return 0;
}

int main(int argc, char** argv)
{
	option_meta longopts;
	const char* name;
	char* value;

	init_option_meta(&longopts, argc, argv);

	while ( get_next_opt(&longopts, &name, &value) == 0 ) 
	{
		if ( strcasecmp(name, "add-label") == 0 )
		{
			printf("Adding label %s\n", value);
			continue;
		}

		if ( strcasecmp(name, "text") == 0 )
		{
			printf("Text field %s\n", value);
			continue;
		}

		if ( strcasecmp(name, "add-entry") == 0 )
		{
			char* default_value;

			printf("Adding entry field %s", value);
			get_optional_default(&longopts, 
					     "entry-default", 
					     &default_value);
			if ( default_value != NULL )
				printf(" with default value %s", default_value);
			printf("\n");

			continue;
		}

		printf("Error: Option %s is not understood\n", name);
		exit(1);
	}

	if ( optind < argc )
	{
		printf("Remaining arguments: ");
		while ( optind < argc )
		{
			printf("%s ", argv[optind++]);
		}
		printf("\n");
	}

	return 0;
}

/* 
 * https://developer.gnome.org/gtk3/stable/ch03.html

A remake of the zenity --form option with a bit more functionality.  

Usage:
--text="text"			Shown on the top of the window
--show-header			Show the column names
--separator="char"		Set the output separater character, default '|'
--add-label="Label"		Plain text, useful for comments or headings
--add-separator			A simple line, for breaking up sections
--add-image="/path/to/image"	A graphic, doesn't return anything

--add-entry="Label"		Textbox field
  --entry-default="Text"	Default fill
--add-password"Label"		Password field
  --password-default="Text"	Default password (note: user won't see it!)
--add-calendar="Label"		Calendar field -- lets the user select a day
--add-checkbox="Label"		Checkbox field
  --checked			It is selected by default
--add-combo="Label"		Add a combobox
  --combo-values="a|b|c"	Separated by |
  --combo-default="a"		If unset there will be no default
--add-file-selector="Label"	Chooses a file or directory
  --file-default="filename"
--add-slider="Label		A Slider, returns a value from 0 to 100
  --slider-default="#"		Should be a value from 0 to 100
--add-font="Label"		Font selector
--add-color="Label"		Color selector
--add-application="Label"	Application selector
--add-page-setup="Label"	Page Setup (for printing)
--add-print="Label"		Print Dialog
*/
