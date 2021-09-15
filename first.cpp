#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>

int main(int argc, char **argv) {
	Fl_Window *window = new Fl_Window(300, 180);
	Fl_Box *box = new Fl_Box(20, 40, 260, 100, "Hello, World!");
	box->box(FL_UP_BOX);
	box->labelsize(36);
	// box->labelfont(FL_BOLD + FL_ITALIC);
	box->labeltype(FL_SHADOW_LABEL);


	// Fl_Button *button = new Fl_Button(0, 0, 260, 100, "label");

	window->end();
	window->show(argc, argv);
	return Fl::run();
}