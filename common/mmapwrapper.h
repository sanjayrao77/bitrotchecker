
struct mmapwrapper {
	void *addr,*freeaddr,*unmapaddr;
	uint64_t addrlength,filesize;
};
H_CLEARFUNC(mmapwrapper);

#define OFFSET_MMAP(a,b) ((unsigned char *)(a)->addr+(b))

int initreadfile_mmapwrapper(struct mmapwrapper *m, char *filename);
void deinit_mmapwrapper(struct mmapwrapper *mmap);
int initreadfd_mmapwrapper(struct mmapwrapper *m, int fd);
int slurpfd_mmapwrapper(struct mmapwrapper *m, int fd);
int initreadfd2_mmapwrapper(struct mmapwrapper *m, int fd, uint64_t filesize);
