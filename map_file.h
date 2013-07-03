#ifndef _MAP_FILE_H
#define _MAP_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *sp, *ep;
} ptr_range;

ptr_range map_input_file(const char *filename);
void map_input_file2(const char *filename, void **start, long *len);
void unmap_file(ptr_range extent);

#define map_in(ptr, len, path) map_input_file2(path, (void**)&ptr, &len), len /= sizeof(*ptr)

#ifdef __cplusplus
}
#endif

#endif
