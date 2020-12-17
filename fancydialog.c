#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>

#include <gtk/gtk.h>

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
	{"checked",		required_argument,	0,	0},
	{"add-combobox",	required_argument,	0,	0},
	{"combo-value",		required_argument,	0,	0},
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
	char*(*get_item_text)(GtkWidget* widget);
	GtkWidget* widget;
} textconverters;

typedef struct
{
	int argc;
	char** argv;
	const char* previous_option;
	char* previous_optarg;
	int previous_hasarg;
	int option_queued;
} option_meta;

/***************************************************************************/
/* Each field type needs a "getter" that returns a basic string when given
 * the widget.
 */

char* entry_to_text(GtkWidget* entry)
{
	return (char*)gtk_entry_get_text(GTK_ENTRY(entry));
}

char* check_button_to_text(GtkWidget* entry)
{
	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)) == TRUE )
		return "True";
	else
		return "False";
}

char* combobox_to_text(GtkWidget* entry)
{
	char* response;

	/* We're supposed to g_free the response when we're done with it
	 * but the program will be over at that point so screw it.
	 */
	response = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry));
	if ( response == NULL )
		return "(Unselected)";
	else
		return response;
}

/***************************************************************************/

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

/* Returns 1 if an optional argument was detected, 0 if not */
int get_optional_default(option_meta* meta, const char** name, char** value)
{
	*name = NULL;
	*value = NULL;

	if ( get_next_opt(meta, name, value) == 0 )
	{
		if ( strncasecmp(*name, "add-", 4) != 0 )
		{
			*value = NULL;
			meta->option_queued = 1;
			return 0;
		}
		return 1;
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

void ok_click(GtkWidget* button, gpointer nothing)
{
	int* response;
	response = (int*)nothing;
	*response = 0;
	gtk_main_quit();
	return;
}

void cancel_click(GtkWidget* button, gpointer nothing)
{
	int* response;
	response = (int*)nothing;
	*response = 1;
	gtk_main_quit();
	return;
}

int add_input_to_layout(GtkWidget* layout, char* labeltext, GtkWidget* input)
{
	GtkWidget* input_layout;
	GtkWidget* label;

	label = gtk_label_new(labeltext);
	input_layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

	gtk_box_pack_start(GTK_BOX(input_layout), label, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX(input_layout), input, FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX(layout), input_layout, FALSE, FALSE, 4);

	return 0;
}


int main(int argc, char** argv)
{
	option_meta longopts;
	const char* name;
	char* value;
	int response;

	GtkWidget* window;
	GtkWidget* layout;

	textconverters* fields;

	gtk_init(&argc, &argv);

	/* Not every arg will be a converter, but this guarantees
	 * that we will have enough space for all of them.
	 */
	fields = malloc(sizeof(textconverters) * argc);
	memset(fields, 0, sizeof(textconverters) * argc);

	if ( fields == NULL )
	{
		perror("malloc textconverters");
		exit(1);
	}
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GdkRectangle screensize;
	gdk_monitor_get_workarea(gdk_display_get_primary_monitor(gdk_display_get_default()),
				&screensize);

	layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	init_option_meta(&longopts, argc, argv);

	// You might be tempted to use optind here, but it gets
	// all messed up when we look forward to see if the -default
	// option was specified.
	int wi = 0;
	while ( get_next_opt(&longopts, &name, &value) == 0 ) 
	{
		wi++;
		if ( strcasecmp(name, "add-label") == 0 )
		{
			fields[wi].widget = gtk_label_new(value);
			gtk_label_set_xalign(GTK_LABEL(fields[wi].widget), 0.0);
			gtk_box_pack_start(GTK_BOX(layout),
				fields[wi].widget, FALSE, FALSE, 4);
			continue;
		}

		if ( strcasecmp(name, "text") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-entry") == 0 )
		{
			const char* default_name;
			char* default_value;

			fields[wi].widget = gtk_entry_new();

			if ( get_optional_default(&longopts, &default_name, &default_value) )
			{
				if ( strcasecmp(default_name, "entry-default") == 0 )
				{
					gtk_entry_set_text(GTK_ENTRY(fields[wi].widget), default_value);
				}
				else
				{
					fprintf(stderr, "Error: '%s' is not a valid option to 'add-entry'\n", default_name);
					exit(-1);
				}
			}

			fields[wi].get_item_text = &entry_to_text;
			add_input_to_layout(layout, value, fields[wi].widget);
			continue;
		}

		if ( strcasecmp(name, "add-password") == 0 )
		{
			fields[wi].widget = gtk_entry_new();
			gtk_entry_set_visibility(
				GTK_ENTRY(fields[wi].widget), FALSE);
			fields[wi].get_item_text = &entry_to_text;
			add_input_to_layout(layout, value, fields[wi].widget);
			continue;
		}

		if ( strcasecmp(name, "add-calendar") == 0 )
		{
			continue;
		}
		if ( strcasecmp(name, "add-checkbox") == 0 )
		{
			const char* opt_name;
			char* is_checked;

			fields[wi].widget = gtk_check_button_new();

			if ( get_optional_default(&longopts, &opt_name, &is_checked) )
			{
				if ( strcasecmp(opt_name, "checked") == 0 )
				{
					if ( strcasecmp(is_checked, "false") != 0 && 
					     strcasecmp(is_checked, "0") != 0 && 
					     strcasecmp(is_checked, "no") != 0 )
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fields[wi].widget), TRUE);
					}
				}
				else
				{
					fprintf(stderr, "Error, '%s' is not a valid option for 'add-checkbox'\n", opt_name );
					exit(-1);
				}
			}

			fields[wi].get_item_text = &check_button_to_text;
			add_input_to_layout(layout, value, fields[wi].widget);
			continue;
		}

		if ( strcasecmp(name, "add-combobox") == 0 )
		{
			const char* opt_name;
			char* opt_val;

			fields[wi].widget = gtk_combo_box_text_new();

			while ( get_optional_default(&longopts, &opt_name, &opt_val) )
			{
				if ( strcasecmp(opt_name, "combo-value") == 0 )
				{
					gtk_combo_box_text_append_text(
						GTK_COMBO_BOX_TEXT(fields[wi].widget),
						opt_val);
				}
				else if ( strcasecmp(opt_name, "combo-default") == 0 )
				{
					gtk_combo_box_text_append_text(
						GTK_COMBO_BOX_TEXT(fields[wi].widget),
						opt_val);
				}
				else
				{
					fprintf(stderr, "Error, '%s' is not a valid option for 'add-combo'\n", opt_name);
					exit(-1);
				}
			}

			fields[wi].get_item_text = &combobox_to_text;
			add_input_to_layout(layout, value, fields[wi].widget);

			continue;
		}

		if ( strcasecmp(name, "add-file-selector") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-slider") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-font") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-color") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-application") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-page-setup") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-print") == 0 )
		{
			continue;
		}

		printf("Error: Option %s is not supported.\n", name);
		exit(-1);
	}

	if ( optind < argc )
	{
		printf("Unable to process argument '%s'\n", argv[optind]);
		exit(-1);
	}

	/* Create the buttons */
	GtkWidget* buttonlayout;
	GtkWidget* cancelbutton;
	GtkWidget* okbutton;

	buttonlayout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

	okbutton = gtk_button_new_with_label("Ok");
	g_signal_connect(G_OBJECT(okbutton), "clicked", 
			G_CALLBACK(ok_click), &response);
	gtk_box_pack_start(GTK_BOX(buttonlayout), okbutton, FALSE, FALSE, 4);

	cancelbutton = gtk_button_new_with_label("Cancel");
	g_signal_connect(G_OBJECT(cancelbutton), "clicked",
			G_CALLBACK(cancel_click), &response);
	gtk_box_pack_start(GTK_BOX(buttonlayout), cancelbutton, FALSE, FALSE, 4);

	gtk_box_pack_start(GTK_BOX(layout), buttonlayout, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), layout);
	gtk_widget_show_all(window);

	gtk_main();

	if ( response != 0 )
		return response;

	for ( int optnum = 1; optnum < argc; optnum++ )
	{
		if ( fields[optnum].get_item_text == NULL )
			continue;

		char* text;

		text = fields[optnum].get_item_text(fields[optnum].widget);

		if ( text == NULL )
		{
			printf("%d: (NULL)\n", optnum);
		}
		else
		{
			printf("%d: %s\n", optnum, text);
		}
	}

	free(fields);
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
