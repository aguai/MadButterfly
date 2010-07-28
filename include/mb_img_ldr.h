#ifndef __MB_IMG_LDR_H_
#define __MB_IMG_LDR_H_

typedef struct _mb_img_data mb_img_data_t;
typedef struct _mb_img_ldr mb_img_ldr_t;

typedef enum _mb_img_fmt {
    MB_IFMT_DUMMY,
    MB_IFMT_ARGB32,
    MB_IFMT_RGB24,
    MB_IFMT_A8,
    MB_IFMT_A1,
    MB_IFMT_RGB16_565
} mb_img_fmt_t;

/*! \brief Encapsulate image.
 *
 * The format and content of an image is encapsulated by imag_data_t.
 * We hope someday, we can create an abstract backend layer that
 * can deal with image data.
 */
struct _mb_img_data {
    /*! \brief Content of the image. */
    void *content;
    int w, h;
    int stride;			/*!< \brief Number of bytes a row */
    mb_img_fmt_t fmt;
    /*! \brief Release the image that was loaded by the loader. */
    void (*free)(mb_img_data_t *img);
};
#define MB_IMG_DATA_FREE(img) (img)->free(img)

/*! \brief Image loader.
 *
 * An image loader take a ID and find out the corresponding
 * image from filesystem or somewhere.  Image ID is a hierachical
 * structured path.  It is relative to the root of image database.
 * Users of a loader do not need to know where the database is.
 * The location can be configured when a loader been instantiated.
 * But, it is invisible when loading images by an image loader.
 */
struct _mb_img_ldr {
    /*! \brief Load a image with specified ID. */
    mb_img_data_t *(*load)(mb_img_ldr_t *ldr, const char *img_id);
    /*! \brief Free the loader. */
    void (*free)(mb_img_ldr_t *ldr);
};
#define MB_IMG_LDR_FREE(ldr) (ldr)->free(ldr)
#define MB_IMG_LDR_LOAD(ldr, img_id) (ldr)->load(ldr, img_id)

/*! \brief Create a simple image loader.
 *
 * \param img_repository is a repository where images are loaded from.
 * \return NULL for error.
 */
extern mb_img_ldr_t *simple_mb_img_ldr_new(const char *img_repository);

#endif /* __MB_IMG_LDR_H_ */
