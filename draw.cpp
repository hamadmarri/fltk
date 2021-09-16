#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <cmath>
#include <pthread.h>
#include <chrono>
#include <mpfr.h>

#define R 0
#define G 1
#define B 2

#define MAX_ITER 250 //250 //100

#define YSIZE 1200
#define PROPOTION 1.666666667
#define XSIZE (int) (YSIZE * PROPOTION)

#define SCALE_X 4.0
#define SCALE_Y SCALE_X / PROPOTION

#define RIGHT_SHIFT 0
#define TOP_SHIFT 0

#define MPFR_PRECISION 200 // 200

class MyWindow;

struct thread_data {
	unsigned int from;
	unsigned int to;
	MyWindow *instance;
};

class MyWindow : public Fl_Double_Window {
private:
	unsigned char pixbuf[YSIZE][XSIZE][3];
	mpfr_t zoomX;
	mpfr_t zoomY;
	mpfr_t origX;
	mpfr_t origY;

	// FLTK DRAW METHOD
	void draw() {
		fl_draw_image((const uchar*) &pixbuf, 0, 0, XSIZE, YSIZE, 3, XSIZE * 3);
	}

public:
	MyWindow(int w, int h, const char *name = 0)
		: Fl_Double_Window(w , h, name)
	{
		end();
		
		mpfr_init2(zoomX, MPFR_PRECISION);
		mpfr_init2(zoomY, MPFR_PRECISION);
		mpfr_init2(origX, MPFR_PRECISION);
		mpfr_init2(origY, MPFR_PRECISION);

		mpfr_set_d(zoomX, SCALE_X, MPFR_RNDD);
		mpfr_set_d(zoomY, SCALE_Y, MPFR_RNDD);
		mpfr_set_d(origX, RIGHT_SHIFT, MPFR_RNDD);
		mpfr_set_d(origY, TOP_SHIFT, MPFR_RNDD);
	
		RenderImage();
	}
	
	~MyWindow() {
		mpfr_clear(zoomX);
		mpfr_clear(zoomY);
		mpfr_clear(origX);
		mpfr_clear(origY);

		mpfr_free_cache();
	}

	int handle(int e) {
		mpfr_t newX, newY, temp;

		if (e != FL_RELEASE)
			return 0;

		mpfr_init2(newX, MPFR_PRECISION);
		mpfr_init2(newY, MPFR_PRECISION);
		mpfr_init2(temp, MPFR_PRECISION);

		mpfr_set_d(newX, Fl::event_x() - ((double) XSIZE / 2.0), MPFR_RNDD);
		mpfr_set_d(newY, Fl::event_y() - ((double) YSIZE / 2.0), MPFR_RNDD);

		mpfr_mul(newX, newX, zoomX, MPFR_RNDD);
		mpfr_div_d(newX, newX, (double) XSIZE, MPFR_RNDD);

		mpfr_mul(newY, newY, zoomY, MPFR_RNDD);
		mpfr_div_d(newY, newY, (double) YSIZE, MPFR_RNDD);

		mpfr_add(origX, origX, newX, MPFR_RNDD);
		mpfr_add(origY, origY, newY, MPFR_RNDD);

		for (int i = 0; i < 4; ++i) {
			mpfr_div_2ui(temp, zoomX, (SCALE_X - 2.0), MPFR_RNDD);
			mpfr_sub(zoomX, zoomX, temp, MPFR_RNDD);
		}

		mpfr_div_d(zoomY, zoomX, PROPOTION, MPFR_RNDD);
		
		RenderImage();

		return 0;
	}

	// PLOT A PIXEL AS AN RGB COLOR INTO THE PIXEL BUFFER
	void PlotPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
		pixbuf[y][x][R] = r;
		pixbuf[y][x][G] = g;
		pixbuf[y][x][B] = b;
	}

	void convertXAxis(mpfr_t &ecX, unsigned int x) {
		mpfr_set_d(ecX, x, MPFR_RNDD);
		mpfr_sub_d(ecX, ecX, (double) XSIZE / 2.0, MPFR_RNDD);
		mpfr_mul(ecX, ecX, zoomX, MPFR_RNDD);
		mpfr_div_d(ecX, ecX, (double) XSIZE, MPFR_RNDD);
		mpfr_add(ecX, ecX, origX, MPFR_RNDD);
	}

	void convertYAxis(mpfr_t &ecY, unsigned int y) {
		mpfr_set_d(ecY, y, MPFR_RNDD);
		mpfr_sub_d(ecY, ecY, (double) YSIZE / 2.0, MPFR_RNDD);
		mpfr_mul(ecY, ecY, zoomY, MPFR_RNDD);
		mpfr_div_d(ecY, ecY, (double) YSIZE, MPFR_RNDD);
		mpfr_add(ecY, ecY, origY, MPFR_RNDD);
	}

	unsigned int mandelbrot(unsigned int x, unsigned int y, mpfr_t &zR, mpfr_t &zI) {		
		unsigned int iter = 0;
		mpfr_t cR, cI, old_zR, temp;

		mpfr_init2(cR, MPFR_PRECISION);
		mpfr_init2(cI, MPFR_PRECISION);
		mpfr_init2(old_zR, MPFR_PRECISION);
		mpfr_init2(temp, MPFR_PRECISION);

		mpfr_set_d(zR, 0.0, MPFR_RNDD);
		mpfr_set_d(zI, 0.0, MPFR_RNDD);

		convertXAxis(cR, x);
		convertYAxis(cI, y);

		do {
			mpfr_set(old_zR, zR, MPFR_RNDD);

			// zR = (zR * zR) - (zI * zI) + cR;			
			mpfr_mul(zR, zR, zR, MPFR_RNDD);
			mpfr_mul(temp, zI, zI, MPFR_RNDD);
			mpfr_sub(zR, zR, temp, MPFR_RNDD);
			mpfr_add(zR, zR, cR, MPFR_RNDD);

			// zI = (2.0 * old_zR * zI) + cI;
			mpfr_mul_d(zI, zI, 2.0, MPFR_RNDD);
			mpfr_mul(zI, zI, old_zR, MPFR_RNDD);
			mpfr_add(zI, zI, cI, MPFR_RNDD);

			iter++;
			
			mpfr_mul(temp, zR, zR, MPFR_RNDD);
			mpfr_mul(old_zR, zI, zI, MPFR_RNDD);
			mpfr_add(temp, temp, old_zR, MPFR_RNDD);

		// } while ((zR * zR) + (zI * zI) < 4.0 && iter < MAX_ITER);
		} while (mpfr_cmp_d(temp, 4.0) < 0 && iter < MAX_ITER);

		mpfr_clear(cR);
		mpfr_clear(cI);
		mpfr_clear(old_zR);
		mpfr_clear(temp);

		return iter;
	}

	// MAKE A NEW PICTURE IN THE PIXEL BUFFER, SCHEDULE FLTK TO DRAW IT
	void _RenderImage(unsigned int from, unsigned int to) {
		unsigned char r, g, b;
		unsigned int iter;
		mpfr_t zR, zI;
		int scale_iter = 2;

		mpfr_init2(zR, MPFR_PRECISION);
		mpfr_init2(zI, MPFR_PRECISION);

		for (unsigned int y = from; y < to; y++) {
			for (unsigned int x = 0; x < XSIZE; x++) {
				iter = mandelbrot(x, y, zR, zI);

				r = g = b = 0;

				if (iter != MAX_ITER) {
					r = g = b = 0;
					iter *= scale_iter;
					b = (254 * log(iter) / log(MAX_ITER * scale_iter));					
				}

				PlotPixel(x, y, r, g, b);
			}
		}

		mpfr_clear(zR);
		mpfr_clear(zI);
	}

	static void *thread_wraper(void *context) {
		thread_data *td = (thread_data*) context;

		td->instance->_RenderImage(td->from, td->to);
		return 0;
	}

	void RenderImage() {
		const int number_of_threads = 4;
		pthread_t threads[number_of_threads];
		struct thread_data td[number_of_threads];
		unsigned int from = 0;
		const unsigned int step = YSIZE / number_of_threads;
		unsigned int to = step;

		auto started = std::chrono::high_resolution_clock::now();
		
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

		auto done = std::chrono::high_resolution_clock::now();
		printf("took: %ldms\n",
			std::chrono::duration_cast<std::chrono::milliseconds>(done - started).count());

		redraw();
	}
};

int main(int argc, char**argv) {
	Fl::visual(FL_RGB); // prevents dithering on some systems
	MyWindow *win = new MyWindow(XSIZE, YSIZE);
	win->show();
	return Fl::run();
}