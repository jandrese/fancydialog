#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>

#include <gtk/gtk.h>

static struct option main_options[] =
{
	{"help",		no_argument,		0,	0},
	{"title",		required_argument,	0,	0},
	{"icon",		required_argument,	0,	0},
	{"iconfile",		required_argument,	0,	0},
	{"separator",		required_argument,	0,	0},
	{"add-label",		required_argument,	0,	0},
	{"add-separator",	no_argument,		0,	0},
	{"add-text",		required_argument,	0,	0},
	{"add-image",		required_argument,	0,	0},
	{"add-entry",		required_argument,	0,	0},
	{"add-password",	required_argument,	0,	0},
	{"add-calendar",	required_argument,	0,	0},
	{"add-checkbox",	required_argument,	0,	0},
	{"add-switch",		required_argument,	0,	0},
	{"add-combobox",	required_argument,	0,	0},
	{"add-file-selector",	required_argument,	0,	0},
	{"add-slider",		required_argument,	0,	0},
	{"add-font",		required_argument,	0,	0},
	{"add-color",		required_argument,	0,	0},
	{"add-application",	required_argument,	0,	0},
/*	
 	{"add-page-setup",	required_argument,	0,	0},
	{"add-print",		required_argument,	0,	0},
 */
	{0,			0,			0,	0},
};

static struct option entry_options[] =
{
	{"default",		required_argument,	0,	0},
	{"width",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

static struct option checkbox_options[] =
{
	{"checked",		no_argument,		0,	0},
	{0,			0,			0,	0},
};

static struct option label_options[] =
{
	{"style",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

static struct option switch_options[] =
{
	{"on",			no_argument,		0,	0},
	{0,			0,			0,	0},
};

static struct option combobox_options[] =
{
	{"value",		required_argument,	0,	0},
	{"default",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

static struct option filechooser_options[] =
{
	{"savefile",		no_argument,		0,	0},
	{"createfolder",	no_argument,		0,	0},
	{"selectfolder",	no_argument,		0,	0},
	{0,			0,			0,	0},
};

static struct option color_options[] =
{
	{"default",		required_argument,	0,	0},
	{"hexstr",		no_argument,		0,	0},
	{0,			0,			0,	0},
};

static struct option font_options[] =
{
	{"default",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

static struct option calendar_options[] =
{
	{"format",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

static struct option slider_options[] =
{
	{"default",		required_argument,	0,	0},
	{"min",			required_argument,	0,	0},
	{"max",			required_argument,	0,	0},
	{"step",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

static struct option application_options[] =
{
	{"file",		required_argument,	0,	0},
	{"mimetype",		required_argument,	0,	0},
	{0,			0,			0,	0},
};

typedef struct
{
	const char* name;
	char*(*get_item_text)(GtkWidget* widget, void* extra);
	void* extra;
	GtkWidget* widget;
} textconverters;

typedef struct
{
	int argc;
	char** argv;
} option_meta;

/***************************************************************************/
/* Each field type needs a "getter" that returns a basic string when given
 * the widget.
 */

char* entry_to_text(GtkWidget* entry, void* extra)
{
	return (char*)gtk_entry_get_text(GTK_ENTRY(entry));
}

char* textbox_to_text(GtkWidget* entry, void* extra)
{
	GtkTextBuffer* buffer;
	GtkTextIter start;
	GtkTextIter end;
	gchar* rawtext;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry));
	gtk_text_buffer_get_iter_at_offset(buffer, &start, 0);
	gtk_text_buffer_get_iter_at_offset(buffer, &end, -1);

	rawtext = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	return g_base64_encode((guchar*)rawtext, strlen(rawtext));
}

char* check_button_to_text(GtkWidget* entry, void* extra)
{
	if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)) == TRUE )
		return "True";
	else
		return "False";
}

char* switch_to_text(GtkWidget* entry, void* extra)
{
	if ( gtk_switch_get_state(GTK_SWITCH(entry)) == TRUE )
		return "On";
	else
		return "Off";
}

char* combobox_to_text(GtkWidget* entry, void* extra)
{
	return gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry));
}

char* file_chooser_to_text(GtkWidget* entry, void* extra)
{
	return gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(entry));
}

char* color_button_to_text(GtkWidget* entry, void* extra)
{
	GdkRGBA colors;
	char colorstr[32];
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(entry), &colors);
	sprintf(colorstr, "%.7f,%.7f,%.7f,%.7f",
		colors.red, colors.green, colors.blue, colors.alpha);
	return strdup(colorstr);
}

char* color_button_to_hex(GtkWidget* entry, void* extra)
{
	GdkRGBA colors;
	char colorstr[32];
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(entry), &colors);
	sprintf(colorstr, "#%02x%02x%02x%02x",
		(uint8_t)(colors.red * 255L), (uint8_t)(colors.green * 255L), 
		(uint8_t)(colors.blue * 255L), (uint8_t)(colors.alpha * 255L));
	return strdup(colorstr);
}

char* font_chooser_to_text(GtkWidget* entry, void* extra)
{
	return gtk_font_chooser_get_font(GTK_FONT_CHOOSER(entry));
}

char* slider_to_text(GtkWidget* entry, void* extra)
{
	char* valuestr;

	valuestr = malloc(sizeof(char*) * 32);
	sprintf(valuestr, "%f", gtk_range_get_value(GTK_RANGE(entry)));
	return valuestr;
}	

char* app_chooser_to_text(GtkWidget* entry, void* extra)
{
	GAppInfo* info;

	info = gtk_app_chooser_get_app_info(GTK_APP_CHOOSER(entry));

	if ( info == NULL )
		return "UNSET";

	if ( g_app_info_get_commandline(info) != NULL )
		return (char*)g_app_info_get_commandline(info);
	else if ( g_app_info_get_executable(info) )
		return (char*)g_app_info_get_executable(info);
	else
		return "UNKNOWN";
}

char* calendar_to_text(GtkWidget* entry, void* extra)
{
	guint year;
	guint month;
	guint day;

	gtk_calendar_get_date(GTK_CALENDAR(entry), &year, &month, &day);

	GTimeZone* tz;
	GDateTime* usertime;

	tz = g_time_zone_new("Z");

	usertime = g_date_time_new(tz, year, month + 1, day, 0, 0, 0);

	if ( extra != NULL )
		return g_date_time_format(usertime, (char*)extra);
	else
		return g_date_time_format(usertime, "%d/%m/%Y");
}

/***************************************************************************/

int get_next_opt(option_meta* meta, const char** option, char** value)
{
	int optval;
	int index;
	
	optval = getopt_long(meta->argc, meta->argv, "",
			main_options, &index);

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

	*option = main_options[index].name;
	*value = optarg;

	return 0;
}

/* Returns 1 if an optional argument was detected, 0 if not */
int get_optional_default(option_meta* meta, struct option* possible_options, 
			const char** name, char** value)
{
	int optval;
	int index;

	*name = NULL;
	*value = NULL;

	opterr = 0;	/* Don't pollute stderr with spurious messages */
	optval = getopt_long(meta->argc, meta->argv, "",
			possible_options, &index);
	opterr = 1;

	if ( optval < 0 )
		return optval;

	if ( optval == '?' )
	{
		optind--;	// Whoops, getopt_long doesn't reset this when
				// it fails to parse an argument.
		return 0;
	}

	if ( optval > 0 )
	{
		fprintf(stderr, "getopt_long returned an unexpected value %d.  Logic bug in program.\n", optval);
		return -1;
	}	       

	*name = possible_options[index].name;
	*value = optarg;

	return 1;
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

int add_input_to_layout(GtkWidget* layout, char* labeltext, GtkWidget* input, int* row)
{
	GtkWidget* label;

	label = gtk_label_new(labeltext);

	gtk_label_set_xalign(GTK_LABEL(label), 1.0);

	gtk_grid_attach(GTK_GRID(layout), label, 0, *row, 1, 1); 

	gtk_widget_set_hexpand(input, FALSE);
	gtk_widget_set_halign(input, GTK_ALIGN_START);

	gtk_grid_attach(GTK_GRID(layout), input, 1, *row, 1, 1);

	*row += 1;

	return 0;
}

void parse_colorstr(char* str, GdkRGBA* colors)
{
	if ( str[0] == '#' )
	{
		int size;
		int scalefactor;
		long red;
		long green;
		long blue;
		long alpha;
		char parsestr[22];

		size = strlen(str) - 1;
		if ( size % 4 != 0 )
		{
			fprintf(stderr, "Error: Unable to parse color %s: Uneven string size\n", str);
			return;
		}
		if ( size > 16 )
		{
			fprintf(stderr, "Error: Unable to parse color %s: Too much precision\n", str);
			return;
		}

		sprintf(parsestr, "#%%0%d%%0%d%%0%d%%0%d", 
			size / 4, size / 4, size / 4, size / 4);

		sscanf(str, parsestr, &red, &green, &blue, &alpha);

		scalefactor = (1 << size) - 1;
		colors->red = red / scalefactor;
		colors->green = green / scalefactor; 
		colors->blue = blue / scalefactor; 
		colors->alpha = alpha / scalefactor;
	}
	else
	{
		sscanf(str, "%lf,%lf,%lf,%lf", 
			&colors->red,
			&colors->green,
			&colors->blue,
			&colors->alpha);
	}
	return;
}

void showhelp(char* appname)
{
	printf("Displays a GUI dialog box that users can interact with.\n"
		"Designed to be integrated into shell scripts.\n"
		"\n"
		"Usage:\n"
		"%s [options] <element> [<element>...]\n"
		"Options:\n"
		" --title <string>: Set the title of the main window\n"
		" --position <x,y>: Place the window here, default: center\n"
		" --size <w,h>: Set the window size\n"
		" --icon <icon name>: Set the icon for the window by name\n"
		" --iconfile <file>: Set the icon for the window from a file\n"
		" --separator <string>: In the output use this string to\n"
		"		separate keys from values.  Defaults to '='\n"
		"Elements:\n"
		" --add-label <string>: Adds a label, no return value\n"
		"   --style 'Pango Style String'\n"
		"     See: https://developer.gnome.org/pygtk/stable/pango-markup-language.html\n"
		"     Example: --add-label \"My Fancy Label\" --style 'style=\"bold\" size=\"x-large\"'\n"
		" --add-separator: Adds a horizontal line\n"
		" --add-image <image_file>: Adds an image, the file must be a PNG\n"
		" --add-entry <label>: Adds a one line textbox, returns the contents\n"
		"   --default <text>: The entry field is prefilled with this value\n"
		" --add-password <label>: An entry field where the text is obscured\n"
		" --add-calendar <label>: An entry where you can pick a date\n"
		"   --format <formatstring>: Format of the output. See:\n"
		"   https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format\n"
		" --add-checkbox <label>: A binary entry, returns True if selected, False if not\n"
		"   --checked: The checkbox will be selected by default\n"
		" --add-switch <label>: Another binary entry, this one stylized like an on-off switch, default off\n"
		"   --on: The switch will be in the 'on' position by default\n"
		" --add-combobox <label>: A dropdown menu that allows one selection\n"
		"   --value <text>: This value will be added to the combox\n"
		"                   Use this option multiple times to add additional entries\n"
		"   --default <text>: This value will be added and automatically selected\n"
		" --add-file-selector <label>: Select a file on the filesystem\n"
		"   --savefile: Switch to choosing a new file to save to\n"
		"   --selectfolder: Switch to selecting an existing folder\n"
		"   --createfolder: Switch to createing a new folder\n"
		" --add-slider <label>: Allows the user to select a value between 0 and 100\n"
		"   --default <0-100>: Start at this value\n"
		"   --min <value>: Select a different minimum value\n"
		"   --max <value>: Select a different maximum value\n"
		" --add-font <label>: An entry field where you choose a system font\n"
		"   --default <fontspec>: The default font selected\n"
		" --add-color <label>: An entry field where you choose a color\n"
		"   --default <r,g,b>: The color selected by default\n"
		"   --hexstr: Output a hex string #rrggbb instead of decimal\n"
		" --add-application <label>: An entry where you choose from one of the installed applications\n"
		"   --default <application>: The default application\n",
		appname);
	exit(0);
}

int main(int argc, char** argv)
{
	option_meta longopts;
	const char* name;
	char* value;
	const char* opt_name;
	char* opt_value;
	int response;
	int retval;
	int wi;	/* Widget Index */
	int row;
	char separator[32];

	GtkWidget* window;
	GtkWidget* layout;

	textconverters* fields;
	sprintf(separator, "=");

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

	layout = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(layout), 8);
	gtk_grid_set_column_spacing(GTK_GRID(layout), 8);
	init_option_meta(&longopts, argc, argv);

	wi = 0;
	row = 0;
	while ((retval = get_next_opt(&longopts, &name, &value)) == 0 ) 
	{
		wi++;
		fields[wi].name = value;

		if ( strcasecmp(name, "help") == 0 )
			showhelp(argv[0]);

		if ( strcasecmp(name, "separator") == 0 )
		{
			strncpy(separator, value, 31);
			continue;
		}

		if ( strcasecmp(name, "title") == 0 )
		{
			gtk_window_set_title(GTK_WINDOW(window), value);
			continue;
		}

		if ( strcasecmp(name, "icon") == 0 )
		{
			gtk_window_set_icon_name(GTK_WINDOW(window), value);
			continue;
		}

		if ( strcasecmp(name, "iconfile") == 0 )
		{
			GError* gerr = NULL;
			gtk_window_set_icon_from_file(GTK_WINDOW(window), value, &gerr);

			if ( gerr != NULL )
			{
				fprintf(stderr, "Failed to load icon file %s: %s\n", value, gerr->message);
				g_error_free(gerr);
			}

			continue;
		}

		if ( strcasecmp(name, "add-label") == 0 )
		{
			fields[wi].widget = gtk_label_new(value);
			gtk_label_set_xalign(GTK_LABEL(fields[wi].widget), 0.0);

			while ( get_optional_default(&longopts, label_options,
							&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "style") == 0 )
				{
					const char* format = "<span %s>%s</span>";
					char* formatted;
					formatted = malloc(sizeof(char) * (strlen(format) + strlen(value) + strlen(opt_value) + 1));
					sprintf(formatted, format, opt_value, value);
						  	
					gtk_label_set_markup(GTK_LABEL(fields[wi].widget), formatted);
					free(formatted);
				}

			gtk_grid_attach(GTK_GRID(layout), fields[wi].widget, 0, row++, 2, 1);
			continue;
		}

		if ( strcasecmp(name, "add-separator") == 0 )
		{
			fields[wi].widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(layout), fields[wi].widget, 0, row++, 2, 1);
			continue;
		}

		if ( strcasecmp(name, "add-text") == 0 )
		{
			fields[wi].widget = gtk_text_view_new();

			fields[wi].get_item_text = &textbox_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-image") == 0 )
		{
			fields[wi].widget = gtk_image_new_from_file(value);
			gtk_grid_attach(GTK_GRID(layout), fields[wi].widget,
					1, row++, 1, 1);
			continue;
		}

		if ( strcasecmp(name, "add-entry") == 0 )
		{
			const char* opt_name;
			char* opt_value;

			fields[wi].widget = gtk_entry_new();

			while ( get_optional_default(&longopts, entry_options, 
							&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "default") == 0 )
					gtk_entry_set_text(GTK_ENTRY(fields[wi].widget), opt_value);
				else if ( strcasecmp(opt_name, "width") == 0 )
					gtk_entry_set_width_chars(GTK_ENTRY(fields[wi].widget),
								strtol(opt_value, NULL, 10));

			fields[wi].get_item_text = &entry_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-password") == 0 )
		{
			fields[wi].widget = gtk_entry_new();
			gtk_entry_set_visibility(
				GTK_ENTRY(fields[wi].widget), FALSE);
			fields[wi].get_item_text = &entry_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-calendar") == 0 )
		{
			fields[wi].widget = gtk_calendar_new();
			fields[wi].get_item_text = &calendar_to_text;

			while ( get_optional_default(&longopts,
				calendar_options,
				&opt_name, &opt_value) == 1 )
			{
				if ( strcasecmp(opt_name, "format") == 0 )
				{
					fields[wi].extra = strdup(opt_value);
				}
			}
			add_input_to_layout(layout, value, fields[wi].widget, &row);

			continue;
		}
		if ( strcasecmp(name, "add-checkbox") == 0 )
		{
			fields[wi].widget = gtk_check_button_new();

			while ( get_optional_default(&longopts, checkbox_options,
						&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "checked") == 0 )
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fields[wi].widget), TRUE);

			fields[wi].get_item_text = &check_button_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-switch") == 0 )
		{
			fields[wi].widget = gtk_switch_new();

			while ( get_optional_default(&longopts, switch_options,
						&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "on") == 0 )
					gtk_switch_set_state(GTK_SWITCH(fields[wi].widget), TRUE);

			fields[wi].get_item_text = &switch_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-combobox") == 0 )
		{
			fields[wi].widget = gtk_combo_box_text_new();

			while ( get_optional_default(&longopts, combobox_options, 
						&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "value") == 0 )
					gtk_combo_box_text_append_text(
						GTK_COMBO_BOX_TEXT(fields[wi].widget),
						opt_value);
				else if ( strcasecmp(opt_name, "default") == 0 )
					gtk_combo_box_text_append_text(
						GTK_COMBO_BOX_TEXT(fields[wi].widget),
						opt_value);

			fields[wi].get_item_text = &combobox_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);

			continue;
		}

		if ( strcasecmp(name, "add-file-selector") == 0 )
		{
			GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

			while ( get_optional_default(&longopts,
						filechooser_options,
						&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "savefile") == 0 )
					action = GTK_FILE_CHOOSER_ACTION_SAVE;
				else if ( strcasecmp(opt_name, "selectfolder") == 0 )
					action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
				else if ( strcasecmp(opt_name, "createfolder") == 0 )
					action = GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;

			fields[wi].widget = gtk_file_chooser_button_new(value, action);
			fields[wi].get_item_text = &file_chooser_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-font") == 0 )
		{
			fields[wi].widget = gtk_font_button_new();

			while ( get_optional_default(&longopts,
						font_options,
						&opt_name, &opt_value) == 1)
				if ( strcasecmp(opt_name, "default" ) == 0 )
					gtk_font_chooser_set_font(GTK_FONT_CHOOSER(fields[wi].widget), opt_value);

			fields[wi].get_item_text = &font_chooser_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			continue;
		}

		if ( strcasecmp(name, "add-slider") == 0 )
		{
			double min = 0;
			double max = 100;
			double step = 1;
			double defaultvalue = 0;

			while ( get_optional_default(&longopts,
						slider_options,
						&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "min") == 0 )
					min = strtod(opt_value, NULL);
				else if ( strcasecmp(opt_name, "max") == 0 )
					max = strtod(opt_value, NULL);
				else if ( strcasecmp(opt_name, "step") == 0 )
					step = strtod(opt_value, NULL);
				else if ( strcasecmp(opt_name, "default") == 0 )
					defaultvalue = strtod(opt_value, NULL);

			if ( defaultvalue < min )
				defaultvalue = min;
			if ( defaultvalue > max )
				defaultvalue = max;

			if ( (min >= max) || ((max - min) < step) )
			{
				fprintf(stderr, "Error: slider configuration doesn't make sense.  Min = %lf, Default = %lf, Max = %lf, Step = %lf.\n"
						"Parameters must satisfy: min <= default <= max, step <= (max - min)\n", min, defaultvalue, max, step);
				exit(-1);
			}
			
			fields[wi].widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
								min, max, step);
			gtk_range_set_value(GTK_RANGE(fields[wi].widget), defaultvalue);

			fields[wi].get_item_text = &slider_to_text;

			/*
			GtkStyleContext* slider_style_context;
			slider_style_context = gtk_widget_get_style_context(fields[wi].widget);
			GtkCssProvider* width_provider;
			width_provider = gtk_css_provider_new();
			GError* gerr = NULL;
			gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(width_provider),
							"slider { min-width: 200px }\n", -1, &gerr);
			if ( gerr != NULL )
			{
				fprintf(stderr, "Parsing CSS string: %s\n", gerr->message);
				g_error_free(gerr);
			}
			else
			{
				gtk_style_context_add_provider(slider_style_context, 
					GTK_STYLE_PROVIDER(width_provider), 
					GTK_STYLE_PROVIDER_PRIORITY_USER);
			}
			*/
			gtk_widget_set_size_request(fields[wi].widget, 200, -1);

			add_input_to_layout(layout, value, fields[wi].widget, &row);

			continue;
		}

		if ( strcasecmp(name, "add-color") == 0 )
		{
			fields[wi].widget = gtk_color_button_new();
			gtk_color_button_set_title(GTK_COLOR_BUTTON(fields[wi].widget), value);

			while ( get_optional_default(&longopts,
						color_options,
						&opt_name, &opt_value) == 1 )
			{
				if ( strcasecmp(opt_name, "default") == 0 )
				{
					GdkRGBA default_color;
					parse_colorstr(opt_value, &default_color);
					gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(fields[wi].widget), &default_color);
				}
				else if ( strcasecmp(opt_name, "hexstr") == 0 )
				{
					fields[wi].get_item_text = &color_button_to_hex;
				}
			}

			fields[wi].get_item_text = &color_button_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);
			
			continue;
		}

		if ( strcasecmp(name, "add-application") == 0 )
		{

			char* content_type = NULL;

			while ( get_optional_default(&longopts,
						application_options,
						&opt_name, &opt_value) == 1 )
				if ( strcasecmp(opt_name, "file") == 0 )
				{
					content_type = g_content_type_guess(opt_value, NULL, 0, NULL);
				}
				else if ( strcasecmp(opt_name, "mimetype") == 0 )
				{
					content_type = strdup(opt_value);
				}


			fields[wi].widget = gtk_app_chooser_button_new(content_type);
			fields[wi].get_item_text = &app_chooser_to_text;
			add_input_to_layout(layout, value, fields[wi].widget, &row);

			continue;
		}

		/*
		if ( strcasecmp(name, "add-page-setup") == 0 )
		{
			continue;
		}

		if ( strcasecmp(name, "add-print") == 0 )
		{
			continue;
		}
		*/

		printf("Error: Option %s is not supported.\n", name);
		exit(-1);
	}

	if ( optind < argc )
	{
		printf("Unable to process argument '%s', got '%d'\n",
		argv[optind], retval);
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

	gtk_grid_attach(GTK_GRID(layout), buttonlayout, 1, row, 1, 1);

	/* Stuff the grid into a box so I can pad around the edges */
	GtkWidget* paddingbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(paddingbox), layout, FALSE, FALSE, 8);

	gtk_container_add(GTK_CONTAINER(window), paddingbox);
	gtk_widget_show_all(window);

	gtk_main();

	if ( response != 0 )
		return response;

	for ( int optnum = 1; optnum < argc; optnum++ )
	{
		if ( fields[optnum].get_item_text == NULL )
			continue;

		char* text = fields[optnum].get_item_text(fields[optnum].widget,
					     fields[optnum].extra);

		printf("%s%s%s\n", fields[optnum].name, separator, text);
	}

	free(fields);
	return 0;
}

/* 
 * https://developer.gnome.org/gtk3/stable/ch03.html
 */
