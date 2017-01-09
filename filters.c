#include "filters.h"

/* Verifica tipul imaginii din fisier */
enum filter_types check_type(char *str_type)
{
	if (!strcmp(str_type, "sharpen")) {
		return SHARPEN;
	} else if (!strcmp(str_type, "smooth")) {
		return SMOOTH;
	} else if (!strcmp(str_type, "mean_removal")) {
		return MEAN_REMOVAL;
	} else if (!strcmp(str_type, "blur")) {
		return BLUR;
	}
	return SHARPEN;
}

/* Pe o matrice de pixeli 3 x 3 aplica filtrul primit ca argument */
int conv_matrix(int **pixel, Filter filter)
{
	int sum = 0;
	for (int i = 0; i < FILTER_SIZE; ++i) {
		for (int j = 0; j < FILTER_SIZE; ++j) {
			sum += pixel[i][j] * filter.matrix[i][j];
		}
	}
	sum /= filter.fraction;
	sum += filter.offset;
	if (sum < 0)
		sum = 0;
	if (sum > 255)
		sum = 255;
	return sum;
}

int modify_pixel(int **pixel, enum filter_types filter_type)
{
	if (filter_type == SMOOTH) {
		return conv_matrix(pixel, smooth_filter);
	} else if (filter_type == BLUR) {
		return conv_matrix(pixel, blur_filter);
	} else if (filter_type == SHARPEN) {
		return conv_matrix(pixel, sharpen_filter);
	} else if (filter_type == MEAN_REMOVAL) {
		return conv_matrix(pixel, mean_removal_filter);
	} else if (filter_type == EMBOSS) {
		return conv_matrix(pixel, emboss_filter);
	} else if (filter_type == EDGE_DETECTION) {
		return conv_matrix(pixel, edge_detection_filter);
	} else {
		return 0;
	}
}

/* Parseaza imaginea. */
Image *parse_image(char *file_name)
{
	char buffer[255];
	FILE *file = fopen(file_name, "r");
	if (file == NULL) {
		printf("Bad file name");
		exit(1);
	}
	Image *image = (Image *) malloc(sizeof(Image));

	fgets(&(buffer[0]), 255, file);
	fgets(&(buffer[0]), 255, file);

	fscanf(file, "%d %d", &image->width, &image->height);
	int dummy;
	fscanf(file, "%d", &dummy);	/* Citeste valoarea maxima a unui pixel */

	image->height += 2;
	image->width += 2;
	image->pixels = alloc_2d_int(image->height, image->width);
	for (int i = 1; i < image->height - 1; ++i) {
		for (int j = 1; j < image->width - 1; ++j) {
			fscanf(file, "%d", &(image->pixels[i][j]));	/* Citeste pixelii */
		}
	}
	/* Bordeaza imaginea */
	for (int i = 0; i < image->height - 1; ++i) {
		image->pixels[i][0] = 0;
		image->pixels[i][image->width - 1] = 0;
	}
	for (int i = 0; i < image->width - 1; ++i) {
		image->pixels[0][i] = 0;
		image->pixels[image->height - 1][i] = 0;
	}

	fclose(file);
	return image;
}

/* Scrie imaginea */
void write_image(int **image_pixels, int width_start, int width_end,
		 int height_start, int height_end, char *output_file)
{
	FILE *output = fopen(output_file, "w");
	fprintf(output,
		"P2\n# CREATOR: GIMP PNM Filter Version 1.1\n%d %d\n255\n",
		width_end - width_start, height_end - height_start);
	for (int i = height_start; i < height_end; ++i) {
		for (int j = width_start; j < width_end; ++j) {
			fprintf(output, "%d\n", image_pixels[i][j]);
		}
	}
	fclose(output);
}

void apply_filter(int **adjacent_matrix, int N, int rank, int parent,
		  char *input_file, char *output_file,
		  enum filter_types filter_type, int *lines_processed)
{
	int child_nodes_size;
	int *child_nodes =
	    get_child_nodes(adjacent_matrix, N, rank, &child_nodes_size,
			    parent);

	int no_echos = 0;
	int **image_pixels, height, width, height_start, height_end;
	if (rank == 0) {
		Image *image = parse_image(input_file);
		image_pixels = image->pixels;
		height = image->height;
		width = image->width;

		height_start = 0;
		height_end = height;
		free(image);
	}
	/* Asteapta ordine de la parinte */
	if (rank != 0) {
		int dimensions[3];
		MPI_Status status;
		MPI_Recv(&(dimensions[0]), 3, MPI_INT, parent,	/* Primeste dimensiunile. */
			 TAG_DIM1, MPI_COMM_WORLD, &status);

		height_start = dimensions[0];
		height_end = dimensions[1];
		width = dimensions[2];

		height = height_end - height_start + 1;
		image_pixels = alloc_2d_int(height, width);
		MPI_Recv(&(image_pixels[0][0]), height * width,	/* Primeste matricea de pixeli */
			 MPI_INT, parent, 2, MPI_COMM_WORLD, &status);
	}

	/* Trimite ordine la nodurile copil */
	if (child_nodes_size > 0) {
		int step = height / child_nodes_size;
		int dimensions[3];
		dimensions[2] = width;
		int current = 0;
		/* Imparte matricea de pixeli */
		if (child_nodes_size == 1) {
			dimensions[0] = height_start;
			dimensions[1] = height_end;
			MPI_Send(&(dimensions[0]), 3, MPI_INT, child_nodes[0],
				 TAG_DIM1, MPI_COMM_WORLD);
			MPI_Send(&(image_pixels[0][0]),
				 (height_end - height_start + 1) * width,
				 MPI_INT, child_nodes[0], 2, MPI_COMM_WORLD);

			no_echos++;
		}
		if (child_nodes_size > 1) {
			dimensions[0] = height_start;
			dimensions[1] = height_start + step + 1;
			MPI_Send(&(dimensions[0]), 3, MPI_INT, child_nodes[0],
				 TAG_DIM1, MPI_COMM_WORLD);
			MPI_Send(&(image_pixels[0][0]),
				 (step + 2) * width, MPI_INT, child_nodes[0],
				 2, MPI_COMM_WORLD);
			no_echos++;
		}

		for (int i = 1; i < child_nodes_size - 1; ++i) {
			dimensions[0] = height_start + i * step;
			dimensions[1] = height_start + (i + 1) * step + 1;
			MPI_Send(&(dimensions[0]), 3, MPI_INT, child_nodes[i],
				 TAG_DIM1, MPI_COMM_WORLD);
			MPI_Send(&(image_pixels[i * step][0]),
				 (step + 2) * width, MPI_INT, child_nodes[i],
				 2, MPI_COMM_WORLD);
			no_echos++;

		}
		// Ultimul nod
		if (child_nodes_size >= 2) {
			dimensions[0] =
			    height_start + (child_nodes_size - 1) * step;
			dimensions[1] = height_end;
			MPI_Send(&(dimensions[0]), 3, MPI_INT,
				 child_nodes[child_nodes_size - 1], TAG_DIM1,
				 MPI_COMM_WORLD);
			// here fixed send dimension
			MPI_Send(&
				 (image_pixels[(child_nodes_size - 1) * step]
				  [0]),
				 (height_end - (child_nodes_size - 1) * step -
				  height_start + 1) * width, MPI_INT,
				 child_nodes[child_nodes_size - 1], 2,
				 MPI_COMM_WORLD);
			no_echos++;
		}
	}

	/* Primeste rezultate de la fiecare copil */
	while (no_echos > 0) {
		int dimensions[3];
		MPI_Status status;
		MPI_Recv(&(dimensions[0]), 3, MPI_INT, MPI_ANY_SOURCE,	/* Primeste dimensiuni */
			 TAG_DIM2, MPI_COMM_WORLD, &status);

		/* Alloc memorie pentru dimensiunile primite */
		int **image_pixels2 =
		    alloc_2d_int(dimensions[1] - dimensions[0] + 1, width);
		/* Primeste matricea de pixeli */
		MPI_Recv(&(image_pixels2[0][0]),
			 (dimensions[1] - dimensions[0] + 1) * dimensions[2],
			 MPI_INT, status.MPI_SOURCE,
			 TAG_RESULT, MPI_COMM_WORLD, &status);

		/* Merge rezultat */
		int k = 1;
		for (int i = dimensions[0] - height_start + 1;
		     i < dimensions[1] - height_start; ++i) {
			for (int j = 0; j < width; ++j) {
				image_pixels[i][j] = image_pixels2[k][j];
			}
			k++;
		}
		/* Dealloc matricea de pixlei primita */
		dealloc_2d_int(image_pixels2);
		no_echos--;
	}

	/* Calculeaza pixeli (nod frunza) noi si/sau trimite mai departe (nod intermediar) */
	if (rank != 0) {
		/* Calculeaza pixeli - frunza */
		if (is_leaf(adjacent_matrix, N, rank)) {
			int **pixel_matrix = alloc_2d_int(3, 3);
			/* Copiaza matricea de pixeli */
			int **image_pixels_copy =
			    alloc_2d_int(height_end - height_start + 1, width);
			for (int i = 0; i < height_end - height_start + 1; ++i) {
				for (int j = 0; j < width; ++j) {
					image_pixels_copy[i][j] =
					    image_pixels[i][j];
				}
			}
			/* Construieste matricea 3x3 si aplica filtrul corespunzator */
			for (int i = 1; i < height_end - height_start; ++i) {
				for (int j = 1; j < width - 1; ++j) {
					pixel_matrix[0][0] =
					    image_pixels_copy[i - 1][j - 1];
					pixel_matrix[0][1] =
					    image_pixels_copy[i - 1][j];
					pixel_matrix[0][2] =
					    image_pixels_copy[i - 1][j + 1];
					pixel_matrix[1][0] =
					    image_pixels_copy[i][j - 1];
					pixel_matrix[1][1] =
					    image_pixels_copy[i][j];
					pixel_matrix[1][2] =
					    image_pixels_copy[i][j + 1];
					pixel_matrix[2][0] =
					    image_pixels_copy[i + 1][j - 1];
					pixel_matrix[2][1] =
					    image_pixels_copy[i + 1][j];
					pixel_matrix[2][2] =
					    image_pixels_copy[i + 1][j + 1];
					image_pixels[i][j] =
					    modify_pixel(pixel_matrix,
							 filter_type);
				}
			}
			lines_processed[rank] += height_end - height_start - 1;
			dealloc_2d_int(pixel_matrix);
			dealloc_2d_int(image_pixels_copy);
		}
		/* Trimite rezultatul catre parinte */
		int dimensions[3];
		dimensions[0] = height_start;
		dimensions[1] = height_end;
		dimensions[2] = width;
		MPI_Send(&(dimensions[0]), 3, MPI_INT, parent,
			 TAG_DIM2, MPI_COMM_WORLD);
		MPI_Send(&(image_pixels[0][0]),
			 (height_end - height_start + 1) * width, MPI_INT,
			 parent, TAG_RESULT, MPI_COMM_WORLD);
	}

	/* Scrie imaginea prelucrata */
	if (rank == 0) {
		write_image(image_pixels, 1, width - 1, 1, height_end - 1,
			    output_file);
	}

	dealloc_1d_int(child_nodes);
	dealloc_2d_int(image_pixels);
}

/* Proceseaza fisierul cu imagini */
int *process_images(char *images_file, int **adjacent_matrix, int dim,
		    int rank, int parent)
{
	int n;
	int *lines_processed = alloc_1d_int(dim);
	FILE *fp = fopen(images_file, "r");
	char filter_type_str[255], file_in[255], file_out[255];

	if (!fp) {
		printf("Images file error.\n");
		exit(1);
	}
	fscanf(fp, "%d\n", &n);
	for (int i = 0; i < n; ++i) {
		fscanf(fp, "%s", filter_type_str);
		fscanf(fp, "%s", file_in);
		fscanf(fp, "%s", file_out);
		enum filter_types filter = check_type(&(filter_type_str[0]));
		apply_filter(adjacent_matrix, dim, rank, parent,
			     file_in, file_out, filter, lines_processed);
	}
	fclose(fp);
	return lines_processed;
}
