#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <cmath>
#include <pthread.h>

#define R 0
#define G 1
#define B 2

#define MAX_ITER 250 //100

#define YSIZE 1200
#define PROPOTION 1.666666667
#define XSIZE (int) (YSIZE * PROPOTION)

#define SCALE_X 4.0
#define SCALE_Y SCALE_X / PROPOTION

#define RIGHT_SHIFT 0
#define TOP_SHIFT 0

// #define UPDATE_RATE 0.016 // 62fps

class MyWindow;

struct thread_data {
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

			zoomX -= zoomX / (SCALE_X - 2.0);
			zoomY = zoomX / PROPOTION;
			
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

	unsigned int mandelbrot(unsigned int x, unsigned int y, long double &zR, long double &zI) {		
		unsigned int iter = 0;
		long double cR, cI, old_zR;
		
		zR = zI = 0.0;
		cR = convertXAxis(x);
		cI = convertYAxis(y);
		
		// printf("cR: %f, cI: %f ", cR, cI);

		do {
			old_zR = zR;
			// zR = (zR * zR) - (zI * zI) + cR;
			// zI = (2.0 * old_zR * zI) + cI;

			zR = (zR * zR) - (zI * zI) + cR;
			zI = (2.0 * old_zR * zI) + cI;
			
			iter++;
		} while ((zR * zR) + (zI * zI) < 4.0 && iter < MAX_ITER);

		return iter;
	}
	
	void colorize(unsigned char &r, unsigned char &g, unsigned char &b
			, long double zR, long double zI)
	{
		// printf("zR: %f, zI: %f\n", zR, zI);
		unsigned int scale = 100000;
		long double max = 4.0 * scale;

		zR = abs(zR * scale);
		zI = abs(zI * scale);

		r = 184 * log(zR) / log(scale * 2);
		g = 184 * log(zI) / log(scale * 2);
		b = 204 * log(sqrt(zR * zR + zI * zI)) / log(sqrt(scale * scale + scale * scale));
		// printf("r: %d, g: %d, b: %d\n", r, g, b);
	}

	// MAKE A NEW PICTURE IN THE PIXEL BUFFER, SCHEDULE FLTK TO DRAW IT
	void _RenderImage(unsigned int from, unsigned int to) {
		unsigned char r, g, b;
		unsigned int iter;
		long double zR, zI;
		int scale_iter = 2;

		for (unsigned int y = from; y < to; y++) {
			for (unsigned int x = 0; x < XSIZE; x++) {
				iter = MyWindow::mandelbrot(x, y, zR, zI);
				
				if (iter != MAX_ITER) {
					r = g = b = 0;
					iter *= scale_iter;
					b = (254 * log(iter) / log(MAX_ITER * scale_iter));
					r = (50 * log(iter * abs(zR)) / log(MAX_ITER * scale_iter));
					g = (50 * log(iter * abs(zI)) / log(MAX_ITER * scale_iter));
					// printf("log(%d): %f, b: %d\n", iter, log(iter), b);
					
				} else {
					colorize(r, g, b, zR, zI);
					// r = g = 0;
					// b = 254;
				}

				PlotPixel(x, y, r, g, b);
			}
		}
	}

	static void *thread_wraper(void *context) {
		thread_data *td = (thread_data*) context;

		td->instance->_RenderImage(td->from, td->to);
		return 0;
	}

	void RenderImage() {
		// Fl::lock();
		const int number_of_threads = 4;
		pthread_t threads[number_of_threads];
		struct thread_data td[number_of_threads];
		unsigned int from = 0;
		const unsigned int step = YSIZE / number_of_threads;
		unsigned int to = step;

		for (int i = 0; i < number_of_threads; ++i) {
			td[i].from = from;
			td[i].to = to;
			td[i].instance = this;
			pthread_create(&threads[i], NULL, &MyWindow::thread_wraper, (void*) &td[i]);

			// printf("%d, %d\n", from, to);
			from = to;
			to += step;
		}

		for (int i = 0; i < number_of_threads; ++i)
			pthread_join(threads[i], NULL);

		redraw();
	}
};

int main(int argc, char**argv) {
	Fl::visual(FL_RGB); // prevents dithering on some systems
	MyWindow *win = new MyWindow(XSIZE, YSIZE);
	win->show();
	return Fl::run();
}