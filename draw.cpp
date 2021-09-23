#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Progress.H>
#include <stdio.h>
#include <cmath>
#include <chrono>
#include <pthread.h>
#include <complex>

#define R 0
#define G 1
#define B 2

#define THREADS 8

#define MAX_ITER 1000 // 1000 254

#define YSIZE 1440
#define PROPOTION 1.77777777 //1.666666667
#define XSIZE (int) (YSIZE * PROPOTION)

#define SCALE_X 4.0
#define SCALE_Y SCALE_X / PROPOTION

#define RIGHT_SHIFT 0
#define TOP_SHIFT 0

#define d_complex std::complex<long double>

// #define UPDATE_RATE 0.016 // 62fps

class MyWindow;

struct thread_data {
	int id;
	unsigned int from;
	unsigned int to;
	MyWindow *instance;
};

class MyWindow : public Fl_Double_Window {
private:
	unsigned char pixbuf[YSIZE][XSIZE][3];
	long double zoomX;
	long double zoomY;
	long double origX;
	long double origY;

	unsigned int progress = 0;

	// FLTK DRAW METHOD
	void draw() {
		fl_draw_image((const uchar*) &pixbuf, 0, 0, XSIZE, YSIZE, 3, XSIZE * 3);
	}

public:
	MyWindow(int w, int h, const char *name = 0)
		: Fl_Double_Window(w , h, name)
	{
		end();

		zoomX = SCALE_X;
		zoomY = SCALE_Y;
		origX = RIGHT_SHIFT;
		origY = TOP_SHIFT;

		RenderImage();
	}

	int handle(int e) {
		if (e == FL_RELEASE) {
			// long double newX = convertXAxis(Fl::event_x());
			long double newX = Fl::event_x() - ((long double) XSIZE / 2.0);
			long double newY = Fl::event_y() - ((long double) YSIZE / 2.0);

			newX = zoomX * newX / (long double) XSIZE;
			newY = zoomY * newY / (long double) YSIZE;

			// printf("x: %d, y: %d\n", Fl::event_x(), Fl::event_y());
			// printf("new x: %f, new y: %f\n", newX, newY);

			origX += newX;
			origY += newY;
			// printf("new origX: %f\n", this->origX);

			for (int i = 0; i < 2; ++i) {
				zoomX -= zoomX / (SCALE_X - 2.0);
				zoomY = zoomX / PROPOTION;
			}

			// printf("zoom: %lf\n", this->zoomX);

			RenderImage();
		}

		return 0;
	}

	// PLOT A PIXEL AS AN RGB COLOR INTO THE PIXEL BUFFER
	void PlotPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
		pixbuf[y][x][R] = r;
		pixbuf[y][x][G] = g;
		pixbuf[y][x][B] = b;
	}

	long double convertXAxis(unsigned int x) {
		long double ecX = x;
		ecX = ecX - ((long double) XSIZE / 2.0);
		ecX = zoomX * ecX / (long double) XSIZE;
		ecX += origX;
		return ecX;
	}

	long double convertYAxis(unsigned int y) {
		long double ecY = y;
		ecY = ecY - ((long double) YSIZE / 2.0);
		ecY = zoomY * ecY / (long double) YSIZE;
		ecY += origY;
		return ecY;
	}

	unsigned int mandelbrot(unsigned int x, unsigned int y, d_complex &z) {
		unsigned int iter = 0;
		d_complex c;

		// printf("cR: %f, cI: %f ", cR, cI);

		z = d_complex(0.0, 0.0);
		c = d_complex(convertXAxis(x), convertYAxis(y));

		do {
			// mandelbrot
			//z = pow(z, 2) + c;

			// heart
			//z = (z + d_complex(sin(real(z)), cos(real(z)))) * z + c;

			// z = (c + z) - pow(z, 3) + c;

			// heaven
			//z = (c + z) - (z + pow(d_complex(sin(real(z)), cos(imag(z))), 3)) + c;

			// if (iter % 2 == 0)
			// z = pow(z, 2) + pow(c, 2);
			// else
			// 	z = pow(z, 5) - pow(c, 2);


			z = pow(z, 2) + c;
			//z = sin(z);

			iter++;
		} while (abs(z) < 2.0 && iter < MAX_ITER);

		return iter;
	}

	void colorize(unsigned char &r, unsigned char &g, unsigned char &b
			, d_complex &z, int iter)
	{
		double hue, sat, val, _r, _g, _b;
		double n;

		r = g = b = 0;

		n = ((double) iter) + 1.0 - (log(log(abs(z))) / log(2));
		// n = (n * 5.9999999); // / 360.0;
		// printf("%Lf\n", n);

		r = (n * 0.5) > 254? 254: (n * 0.5);
		g = (n * 0.2) > 254? 254: (n * 0.2);
		b = n > 254? 254: n;

		n = n / log(MAX_ITER);
		if (n >= 6)
			n = 5.999999999;

		hue = n;
		sat = 1.0;
		val = 1.0 - (b + (3 * 254)) / (4.0 * 254.0);

		Fl_Color_Chooser::hsv2rgb(hue, sat, val, _r, _g, _b);
		r = (r + (_r * 254.0)) / 2;
		g = (g + (_g * 254.0) / 2);
		b = (b + (_b * 254.0) / 2);
	}

	void print_progress() {
		this->progress++;

		if (this->progress % 100 == 0) {
			printf("%d%%\n", (this->progress * 100) / YSIZE);
			this->progress++;
		}
	}

	// MAKE A NEW PICTURE IN THE PIXEL BUFFER, SCHEDULE FLTK TO DRAW IT
	void _RenderImage(int id, unsigned int from, unsigned int to) {
		unsigned char r, g, b;
		unsigned int iter;
		d_complex z;
		unsigned int y, x, yx;

		for (yx = from; yx < to; yx++) {
			y = yx / XSIZE;
			x = yx % XSIZE;
			
			iter = mandelbrot(x, y, z);

			if (iter != MAX_ITER) {
				colorize(r, g, b, z, iter);
			} else {
				r = g = b = 254;
			}

			PlotPixel(x, y, r, g, b);

			if (x + 1 == XSIZE)
				print_progress();
		}
		
		//printf("%d done\n", id);
	}

	static void *thread_wraper(void *context) {
		thread_data *td = (thread_data*) context;

		td->instance->_RenderImage(td->id, td->from, td->to);
		return 0;
	}

	void RenderImage() {
		pthread_t threads[THREADS];
		struct thread_data td[THREADS];
		unsigned int from = 0;
		const unsigned int step = (YSIZE * XSIZE) / THREADS;
		unsigned int to = step;

		auto started = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < THREADS; ++i) {
			td[i].id = i;
			td[i].from = from;
			td[i].to = to;
			td[i].instance = this;
			pthread_create(&threads[i], NULL, &MyWindow::thread_wraper, (void*) &td[i]);

			// printf("%d, %d\n", from, to);
			from = to;
			to += step;
		}

		for (int i = 0; i < THREADS; ++i)
			pthread_join(threads[i], NULL);

		auto done = std::chrono::high_resolution_clock::now();
		printf("took: %ldms\n",
			std::chrono::duration_cast<std::chrono::milliseconds>(
				done - started
			).count());

		redraw();
		this->progress = 0;
	}
};

int main(int argc, char**argv) {

	Fl::visual(FL_RGB); // prevents dithering on some systems
	MyWindow *win = new MyWindow(XSIZE, YSIZE);
	win->show();
	return Fl::run();
}
