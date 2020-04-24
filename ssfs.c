#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>

static const char* root = "/home/osboxes/Documents";
static char key[100] = "9(ku@AW1[Lmvgax6q`5Y2Ry?+sF!^HKQiBXCUSe&0M.b%rI'7d)o4~VfZ*{#:}ETt$3J-zpc]lnh8,GwP_ND|jO";

void encv1(char* mode, char* str, int len) {
	if(!strcmp(str, ".") || !strcmp(str, "..")) return;
	if(!strcmp(mode, "de") && !strstr(str, "/")) return;

	int offset = 0, i, j;

	for(i = len; i >= 0; i--) {
		switch(str[i]) {
			case '/':
				break;
			case '.':
				len = i;
				break;
		}
	}

	if(!strcmp(mode, "de")) offset = len;
	for(i = 0; i < len; i++) {
		if(str[i] == '/') {
			offset = i + 1;
			break;
		}
	}

  for(i = offset; i < len; i++) {
		switch (str[i]) {
			case '/':
				continue;
			default :

				if(!strcmp(mode, "en")) {
					for (j = 0; j < 87; j++) {
		        if(str[i] == key[j]) {
		          str[i] = key[(j + 10) % 87];
		          break;
						}
					}
				}
				if(!strcmp(mode, "de")) {
					for (i = 0; i < 87; i++) {
		        if(str[i] == key[j]) {
							str[i] = key[(j + 77) % 87];
		          break;
						}
					}
				}

		}
  }
}

void logging(char* cmd, const char* desc){
	FILE* log = fopen("/home/osboxes/fs.log", "a");
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  char buffer[80];

  strftime(buffer, 80, "WARNING::%y%m%d-%H:%M:%S::", tm);
	fprintf(log, "%s%s%s\n", buffer, cmd, desc);
	fclose(log);
}

static int _getattr(const char* path, struct stat* stbuf) {
	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));
	char fpath[1000];
	sprintf(fpath, "%s%s",root,path);
	if (lstat(fpath, stbuf)) return -errno;
	return 0;
}

static int _readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));

	char fpath[1000];
	if(!strcmp(path,"/")) {
	path = root;
	sprintf(fpath,"%s",path);
}	else sprintf(fpath, "%s%s",root,path);

	DIR* dp;
	struct dirent* de;
	(void) offset;
	(void) fi;
	if(!(dp = opendir(fpath))) return -errno;

	de = readdir(dp);
	while (de) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if(strstr(path, "encv1_")) encv1("en", de->d_name, strlen(de->d_name));
		if(filler(buf, de->d_name, &st, 0)) break;
	}
	closedir(dp);
	return 0;
}

static int _mkdir(const char* path, mode_t mode) {
	char* p = strstr(path, "encv1_");
	if(p) {
		int len = strlen(p);
		for(int i = len; i >= 0; i--) if(p[i] == '/')	break;
		encv1("en", p, len);
	}

	char fpath[1000];
	if(!strcmp(path,"/")) {
	path = root;
	sprintf(fpath,"%s",path);
}	else sprintf(fpath, "%s%s",root,path);
	if (mkdir(fpath, mode))
		return -errno;
	logging("MKDIR::", path);
	return 0;
}

static int _mknod(const char* path, mode_t mode, dev_t rdev){
	char* p = strstr(path, "encv1_");
	if(p) {
		int len = strlen(p);
		for(int i = len; i >= 0; i--) if(p[i] == '/')	break;
		encv1("de", p, len);
	}

	char fpath[1000];
	if(!strcmp(path,"/")) {
	path = root;
	sprintf(fpath,"%s",path);
	}	else sprintf(fpath, "%s%s",root,path);

	int res;
	if (S_ISREG(mode)) {
		res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)	res = close(res);
	} else if (S_ISFIFO(mode)) res = mkfifo(fpath, mode);
	else res = mknod(fpath, mode, rdev);

	logging("CREAT::", path);
	if (res) return -errno;
	return 0;
}

static int _unlink(const char* path) {
	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));

	char fpath[1000];
	if(!strcmp(path,"/")) {
	path = root;
	sprintf(fpath,"%s",path);
}	else sprintf(fpath, "%s%s",root,path);

	logging("REMOVE::", path);
	if (unlink(fpath)) return -errno;
	return 0;
}

static int _rmdir(const char* path) {
	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));

	char fpath[1000];
	sprintf(fpath, "%s%s",root,path);
	logging("RMDIR::", path);
	if (rmdir(fpath)) return -errno;
	return 0;
}

static int _rename(const char* from, const char* to) {

	char src[1000], dest[1000], dir[1000], str[100];
	sprintf(src, "%s%s", root, from);
	sprintf(dest, "%s%s", root, to);
	int index = 0, len = strlen(dest);
	for(int i = len; i >= 0; i--){
		if(dest[i] == '/'){
			index = i;
			break;
		}
	}
	strncpy(dir, dest, index);
	if(fork()) wait(NULL);
	else execl("/bin/mkdir", "mkdir", "-p", dir, NULL);

	sprintf(str, "%s::%s", from, to);
	logging("RENAME::", str);
	if (rename(src, dest))	return -errno;
	return 0;
}

static int _truncate(const char* path, off_t size) {

	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));
	char fpath[1000];
	sprintf(fpath, "%s%s",root,path);
	if (truncate(fpath, size)) return -errno;
	return 0;
}

static int _open(const char* path, struct fuse_file_info* fi) {

	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));
	char fpath[1000];
	sprintf(fpath, "%s%s",root,path);
	int res = open(fpath, fi->flags);
	if (res) return -errno;
	close(res);
	return 0;
}

static int _read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {

	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));
	char fpath[1000];
	sprintf(fpath, "%s%s",root,path);
	int fd, res;
	(void) fi;

	fd = open(fpath, O_RDONLY);
	if(fd) return -errno;

	res = pread(fd, buf, size, offset);
	if(res) res = -errno;

	close(fd);
	return res;
}

static int _write(const char* path, const char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
	char* p = strstr(path, "encv1_");
	if(p) encv1("de", p, strlen(p));
	char fpath[1000];
	sprintf(fpath, "%s%s",root,path);
	int fd, res;
	(void) fi;

	fd = open(fpath, O_WRONLY);
	if(fd) return -errno;

	logging("WRITE::", path);
	res = pwrite(fd, buf, size, offset);
	if(res) res = -errno;

	close(fd);
	return res;
}


static struct fuse_operations _oper = {
	.getattr = _getattr,
	.readdir = _readdir,
	.read = _read,
	.mkdir = _mkdir,
	.mknod = _mknod,
	.unlink = _unlink,
	.rmdir = _rmdir,
	.rename = _rename,
	.truncate = _truncate,
	.open = _open,
	.read = _read,
	.write = _write,
};

int main(int argc, char* argv[]) {
	umask(0);
	return fuse_main(argc, argv, &_oper, NULL);
}
