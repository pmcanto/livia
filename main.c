// Maria Gabriela B 10409037
// Raphaela Polonis 10408843
// Livia Negrucci Cantowitz 10389419
// Isabella Rodrigues de Oliveira 10357696

#include <SDL2/SDL.h> // Biblioteca principal (versao 2)
#include <SDL2/SDL_image.h> // Extensao para imagens (versao 2)
#include <SDL2/SDL_ttf.h> // Extensao para textos (versao 2)
#include <stdio.h> // Entrada e saida padrao
#include <stdbool.h> // Tipo bool (true/false)
#include <math.h> // Funcoes matematicas

#define HIST_W 400 // Largura da janela do histograma
#define HIST_H 300 // Altura da janela do histograma
#define BTN_W 120 // Largura do botao
#define BTN_H 40 // Altura do botao

// Estrutura que representa um botao interativo
typedef struct {
	SDL_Rect rect; // Retangulo (posicao e tamanho do botao)
	bool hovered; // Indica se o mouse esta sobre o botao
	bool clicked; // Indica se o botao foi clicado
	const char *label; // Texto exibido no botao
} Button;

// ---------- Funcoes utilitarias ----------

// Converte uma imagem colorida para tons de cinza
// (Assume que a superficie ja esta em formato 32-bit ARGB)
void convertToGrayscale(SDL_Surface *surface) {
	if (!surface) return;
	SDL_LockSurface(surface); // Bloqueia a superficie para edicao direta dos pixels

	Uint8 r, g, b;
	Uint32 *pixels = (Uint32 *)surface->pixels;
	for (int y = 0; y < surface->h; y++) {
		for (int x = 0; x < surface->w; x++) {
			Uint32 pixel = pixels[y * surface->w + x]; // Obtem pixel
			SDL_GetRGB(pixel, surface->format, &r, &g, &b); // Extrai componentes RGB
			Uint8 gray = (Uint8)(0.2125 * r + 0.7154 * g + 0.0721 * b); // Formula ponderada
			pixels[y * surface->w + x] = SDL_MapRGB(surface->format, gray, gray, gray); // Substitui pelo cinza
		}
	}

	SDL_UnlockSurface(surface); // Desbloqueia apos edicao
}

// Calcula histograma, media e desvio padrao da imagem
void computeHistogram(SDL_Surface *surface, int hist[256], double *media, double *desvio) {
	for (int i = 0; i < 256; i++) hist[i] = 0; // Zera histograma

	Uint32 *pixels = (Uint32 *)surface->pixels;
	int total = surface->w * surface->h; // Numero total de pixels
	Uint8 r, g, b;

	// Conta frequencias de intensidade (considera ja em escala de cinza)
	for (int i = 0; i < total; i++) {
		SDL_GetRGB(pixels[i], surface->format, &r, &g, &b);
		hist[r]++;
	}

	// Calcula media da intensidade
	double soma = 0;
	for (int i = 0; i < 256; i++) soma += i * hist[i];
	*media = soma / total;

	// Calcula desvio padrao
	double soma2 = 0;
	for (int i = 0; i < 256; i++) soma2 += (i - *media) * (i - *media) * hist[i];
	*desvio = sqrt(soma2 / total);
}

// Aplica equalizacao de histograma (melhora contraste)
SDL_Surface* equalizeHistogram(SDL_Surface *src) {
	int hist[256] = {0}, cdf[256]; // histograma e funcao de distribuicao acumulada
	Uint32 *pixels = (Uint32 *)src->pixels;
	int total = src->w * src->h;
	Uint8 r, g, b;

	// Conta histograma
	for (int i = 0; i < total; i++) {
		SDL_GetRGB(pixels[i], src->format, &r, &g, &b);
		hist[r]++;
	}

	// Calcula CDF
	cdf[0] = hist[0];
	for (int i = 1; i < 256; i++) cdf[i] = cdf[i - 1] + hist[i];

	// Cria copia para saida
	// CORRECAO: SDL_ConvertSurfaceFormat precisa de 3 argumentos no SDL2
	SDL_Surface *dst = SDL_ConvertSurfaceFormat(src, src->format->format, 0);
	Uint32 *pixOut = (Uint32 *)dst->pixels;
	int cdf_min = 0;
	for (int i = 0; i < 256; i++) if (cdf[i] > 0) { cdf_min = cdf[i]; break; }

	// Recalcula valores de pixels
	for (int i = 0; i < total; i++) {
		SDL_GetRGB(pixels[i], src->format, &r, &g, &b);
		int newVal = round(((double)(cdf[r] - cdf_min) / (total - cdf_min)) * 255.0);
		pixOut[i] = SDL_MapRGB(dst->format, newVal, newVal, newVal);
	}

	return dst;
}

// Desenha histograma na janela
void drawHistogram(SDL_Renderer *ren, int hist[256]) {
	SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); // Fundo preto
	SDL_RenderClear(ren);

	// Encontra valor maximo para escalar as barras
	int maxVal = 1;
	for (int i = 0; i < 256; i++) if (hist[i] > maxVal) maxVal = hist[i];

	// Desenha linhas do histograma
	for (int i = 0; i < 256; i++) {
		int h = (hist[i] * (HIST_H - 100)) / maxVal; // Escala altura
		SDL_SetRenderDrawColor(ren, 255, 255, 255, 255); // Branco
		SDL_RenderDrawLine(ren, i * HIST_W / 256, HIST_H, i * HIST_W / 256, HIST_H - h);
	}
}

// ---------- Botao ----------

// Renderiza botao com cor de fundo e texto
void renderButton(SDL_Renderer *ren, Button *btn, TTF_Font *font) {
	if (btn->clicked) SDL_SetRenderDrawColor(ren, 0, 0, 180, 255); // Azul escuro
	else if (btn->hovered) SDL_SetRenderDrawColor(ren, 100, 149, 237, 255); // Azul claro
	else SDL_SetRenderDrawColor(ren, 0, 0, 255, 255); // Azul normal

	SDL_RenderFillRect(ren, &btn->rect); // Preenche retangulo do botao

	SDL_Color white = {255, 255, 255, 255}; // Texto branco
	SDL_Surface *textSurf = TTF_RenderText_Blended(font, btn->label, white);
	SDL_Texture *textTex = SDL_CreateTextureFromSurface(ren, textSurf);
	SDL_Rect dst = {btn->rect.x + (btn->rect.w - textSurf->w)/2,
					btn->rect.y + (btn->rect.h - textSurf->h)/2,
					textSurf->w, textSurf->h};
	SDL_RenderCopy(ren, textTex, NULL, &dst); // Centraliza texto no botao
	SDL_DestroyTexture(textTex);
	SDL_FreeSurface(textSurf);
}

// ---------- Render textos ----------

// Mostra classificacao de luminosidade e contraste
void renderClassification(SDL_Renderer *ren, TTF_Font *font, double media, double desvio) {
	const char *luminosidade;
	if (media < 85) luminosidade = "Imagem escura";
	else if (media < 170) luminosidade = "Imagem media";
	else luminosidade = "Imagem clara";

	const char *contraste;
	if (desvio < 50) contraste = "Contraste baixo";
	else if (desvio < 100) contraste = "Contraste medio";
	else contraste = "Contraste alto";

	SDL_Color yellow = {255, 255, 0, 255}; // Texto amarelo

	// Texto luminosidade
	SDL_Surface *surf1 = TTF_RenderText_Blended(font, luminosidade, yellow);
	SDL_Texture *tex1 = SDL_CreateTextureFromSurface(ren, surf1);
	SDL_Rect dst1 = {10, HIST_H - 80, surf1->w, surf1->h};
	SDL_RenderCopy(ren, tex1, NULL, &dst1);

	// Texto contraste
	SDL_Surface *surf2 = TTF_RenderText_Blended(font, contraste, yellow);
	SDL_Texture *tex2 = SDL_CreateTextureFromSurface(ren, surf2);
	SDL_Rect dst2 = {10, HIST_H - 50, surf2->w, surf2->h};
	SDL_RenderCopy(ren, tex2, NULL, &dst2);

	SDL_DestroyTexture(tex1);
	SDL_FreeSurface(surf1);
	SDL_DestroyTexture(tex2);
	SDL_FreeSurface(surf2);
}

// ---------- Main ----------
int main(int argc, char* argv[]) {
	// Verifica se imagem foi passada como argumento
	if (argc < 2) {
		printf("Uso: %s caminho_da_imagem\n", argv[0]);
		return 1;
	}

	// Inicializacoes basicas
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
	TTF_Init();

	// Carrega imagem
	SDL_Surface *image = IMG_Load(argv[1]);
	if (!image) {
		printf("Erro ao carregar imagem: %s\n", IMG_GetError());
		return 1;
	}

	// Cria versao em escala de cinza
	// Converte para um formato 32-bit (mais seguro para processar)
	// CORRECAO: SDL_ConvertSurfaceFormat precisa de 3 argumentos no SDL2
	SDL_Surface *gray = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);
	if (!gray) {
		printf("Erro ao converter imagem para ARGB8888: %s\n", SDL_GetError());
		SDL_FreeSurface(image);
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
		return 1;
	}
	convertToGrayscale(gray);

	// Cria versao equalizada
	SDL_Surface *equalized = equalizeHistogram(gray);

	// Cria janela principal (imagem)
	SDL_Window *win_main = SDL_CreateWindow("Imagem", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gray->w, gray->h, 0);

	// Pega posicao da principal para posicionar a secundaria ao lado
	int main_x, main_y;
	SDL_GetWindowPosition(win_main, &main_x, &main_y);

	// Cria janela do histograma
	SDL_Window *win_hist = SDL_CreateWindow("Histograma", main_x + gray->w, main_y, HIST_W, HIST_H, 0);

	// Renderizadores para cada janela
	SDL_Renderer *ren_main = SDL_CreateRenderer(win_main, -1, 0);
	SDL_Renderer *ren_hist = SDL_CreateRenderer(win_hist, -1, 0);

	SDL_Texture *tex_main = SDL_CreateTextureFromSurface(ren_main, gray); // Textura inicial (imagem em cinza)

	// Histograma inicial
	int hist[256]; double media, desvio;
	computeHistogram(gray, hist, &media, &desvio);

	// Cria botao centralizado na janela de histograma
	Button btn = {{(HIST_W - BTN_W)/2, 10, BTN_W, BTN_H}, false, false, "Equalizar"};
	// Lembre-se de ter o arquivo "arial.ttf" na pasta "assets"
	TTF_Font *font = TTF_OpenFont("assets/arial.ttf", 16);
	if (!font) {
		printf("Erro ao carregar fonte: %s\n", TTF_GetError());
		// (adicionar limpeza de recursos aqui)
		return 1;
	}

	// Flags de controle
	bool running = true, isEqualized = false;
	SDL_Event e;

	// Loop principal
	while (running) {
		while (SDL_PollEvent(&e)) {
			// No SDL2, clicar no 'X' da janela envia SDL_EVENT_QUIT
			if (e.type == SDL_QUIT) {
				running = false;
			}

			// O evento SDL_EVENT_WINDOW_CLOSE_REQUESTED (do SDL3) foi removido.
			// O SDL_EVENT_QUIT acima ja cuida disso.

			// Verifica hover do botao
			if (e.type == SDL_MOUSEMOTION) {
				if (e.motion.windowID == SDL_GetWindowID(win_hist)) {
					int mx = e.motion.x, my = e.motion.y;
					btn.hovered = (mx >= btn.rect.x && mx <= btn.rect.x+btn.rect.w && my >= btn.rect.y && my <= btn.rect.y+btn.rect.h);
				} else {
					btn.hovered = false;
				}
			}

			// Clique no botao
			if (e.type == SDL_MOUSEBUTTONDOWN && btn.hovered && e.button.windowID == SDL_GetWindowID(win_hist)) {
				btn.clicked = true;
			}
			if (e.type == SDL_MOUSEBUTTONUP && btn.clicked && e.button.windowID == SDL_GetWindowID(win_hist)) {
				btn.clicked = false;
				isEqualized = !isEqualized; // Alterna entre equalizada e original
				if (isEqualized) {
					SDL_DestroyTexture(tex_main);
					tex_main = SDL_CreateTextureFromSurface(ren_main, equalized);
					computeHistogram(equalized, hist, &media, &desvio);
					btn.label = "Original"; // Atualiza texto
				} else {
					SDL_DestroyTexture(tex_main);
					tex_main = SDL_CreateTextureFromSurface(ren_main, gray);
					computeHistogram(gray, hist, &media, &desvio);
					btn.label = "Equalizar";
				}
			}

			// Salva imagem atual se pressionar "S"
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s) {
				SDL_Surface *toSave = isEqualized ? equalized : gray;
				IMG_SavePNG(toSave, "output_image.png");
				printf("Imagem salva como output_image.png\n");
			}
		}

		// Renderiza janela principal (imagem)
		SDL_RenderClear(ren_main);
		SDL_RenderCopy(ren_main, tex_main, NULL, NULL);
		SDL_RenderPresent(ren_main);

		// Renderiza janela histograma
		drawHistogram(ren_hist, hist);
		renderButton(ren_hist, &btn, font);
		renderClassification(ren_hist, font, media, desvio);
		SDL_RenderPresent(ren_hist);

		SDL_Delay(16); // ~60 FPS
	}

	// Libera recursos
	SDL_DestroyTexture(tex_main);
	SDL_DestroyRenderer(ren_main);
	SDL_DestroyRenderer(ren_hist);
	SDL_DestroyWindow(win_main);
	SDL_DestroyWindow(win_hist);
	SDL_FreeSurface(image);
	SDL_FreeSurface(gray);
	SDL_FreeSurface(equalized);
	TTF_CloseFont(font);
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
	return 0;
}