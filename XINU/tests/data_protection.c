#include <darwintest.h>
#include <darwintest_utils.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <IOKit/IOKitLib.h>
#include <Kernel/IOKit/crypto/AppleKeyStoreDefs.h>
#include <Kernel/sys/content_protection.h>

#define CPT_IO_SIZE      4096
#define CPT_AKS_BUF_SIZE 256
#define CPT_MAX_PASS_LEN 64

#define GET_PROT_CLASS(fd) \
	fcntl((fd), F_GETPROTECTIONCLASS)

#define SET_PROT_CLASS(fd, prot_class) \
	fcntl((fd), F_SETPROTECTIONCLASS, (prot_class))

#define KEYSTORECTL_PATH  "/usr/local/bin/keystorectl"
#define KEYBAGDTEST_PATH  "/usr/local/bin/keybagdTest"
#define TEMP_DIR_TEMPLATE "/tmp/data_protection_test.XXXXXXXX"
#define TEST_PASSCODE     "IAmASecurePassword"

int g_fd           = -1;
int g_dir_fd       = -1;
int g_subdir_fd    = -1;
int g_passcode_set = 0;

char g_test_tempdir[PATH_MAX] = TEMP_DIR_TEMPLATE;
char g_filepath[PATH_MAX]     = "";
char g_dirpath[PATH_MAX]      = "";
char g_subdirpath[PATH_MAX]   = "";

int apple_key_store(
	uint32_t command,
	uint64_t * inputs,
	uint32_t input_count,
	void * input_structs,
	size_t input_struct_count,
	uint64_t * outputs,
	uint32_t * output_count
	);
int spawn_proc(char * const command[]);
int supports_content_prot(void);
char* dp_class_num_to_string(int num);
int lock_device(void);
int unlock_device(char * passcode);
int set_passcode(char * new_passcode, char * old_passcode);
int clear_passcode(char * passcode);
int has_passcode(void);
void setup(void);
void cleanup(void);

T_DECL(data_protection,
    "Verify behavior of the various data protection classes") {
	int local_result = -1;
	int new_prot_class = -1;
	int old_prot_class = -1;
	int current_byte = 0;
	char rd_buffer[CPT_IO_SIZE];
	char wr_buffer[CPT_IO_SIZE];

	setup();

	  
                                        
                                     
    
	for (
		new_prot_class = PROTECTION_CLASS_A;
		new_prot_class <= PROTECTION_CLASS_F;
		new_prot_class++
		) {
		T_ASSERT_NE(
			old_prot_class = GET_PROT_CLASS(g_fd),
			-1,
			"Get protection class when locked"
			);
		T_WITH_ERRNO;
		T_ASSERT_NE(
			SET_PROT_CLASS(g_fd, new_prot_class),
			-1,
			"Should be able to change protection "
			"from %s to %s while unlocked",
			dp_class_num_to_string(old_prot_class),
			dp_class_num_to_string(new_prot_class)
			);
	}

	                                                              
#ifndef F_GETDEFAULTPROTLEVEL
#define F_GETDEFAULTPROTLEVEL 79
#endif

	T_WITH_ERRNO;
	T_ASSERT_NE(
		old_prot_class = fcntl(g_fd, F_GETDEFAULTPROTLEVEL),
		-1,
		"Get default protection level for filesystem"
		);

	                                                                       

	  
                                                                      
                                             
    
	close(g_fd);
	unlink(g_filepath);

	                        
	T_WITH_ERRNO;
	T_ASSERT_GE(
		g_fd = open(g_filepath, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC),
		0,
		"Recreate test file"
		);

	                                             
	T_ASSERT_EQ(lock_device(), 0, "*** Lock device ***");

	T_WITH_ERRNO;
	T_ASSERT_EQ(
		SET_PROT_CLASS(g_fd, PROTECTION_CLASS_A),
		-1,
		"Should not be able to change protection "
		"from class D to class A when locked"
		);
	T_ASSERT_EQ(unlock_device(TEST_PASSCODE), 0, "*** Unlock device ***");

	                                                          
	T_WITH_ERRNO;
	T_ASSERT_EQ(
		SET_PROT_CLASS(g_fd, PROTECTION_CLASS_A),
		0,
		"Should be able to change protection "
		"from class D to class A when unlocked"
		);

	close(g_fd);

	T_WITH_ERRNO;
	T_ASSERT_GE(
		g_fd = open(g_filepath, O_RDWR | O_CLOEXEC),
		0,
		"Should be able to open a class A file when unlocked");

	  
                                                                    
                                                       
    
	current_byte = 0;

	while (current_byte < CPT_IO_SIZE) {
		local_result = pwrite(
			g_fd,
			&wr_buffer[current_byte],
			CPT_IO_SIZE - current_byte,
			current_byte
			);

		T_WITH_ERRNO;
		T_ASSERT_NE(
			local_result,
			-1,
			"Should be able to write to "
			"a class A file when unlocked"
			);

		current_byte += local_result;
	}

	current_byte = 0;

	while (current_byte < CPT_IO_SIZE) {
		local_result = pread(
			g_fd,
			&rd_buffer[current_byte],
			CPT_IO_SIZE - current_byte,
			current_byte
			);

		T_WITH_ERRNO;
		T_ASSERT_NE(
			local_result,
			-1,
			"Should be able to read from "
			"a class A file when unlocked"
			);

		current_byte += local_result;
	}

	  
                                                                 
            
    
	T_ASSERT_EQ(lock_device(), 0, "*** Lock device ***");

	T_ASSERT_LE(
		pread(g_fd, rd_buffer, CPT_IO_SIZE, 0),
		0,
		"Should not be able to read from a class A file when locked"
		);

	T_ASSERT_LE(
		pwrite(g_fd, wr_buffer, CPT_IO_SIZE, 0),
		0,
		"Should not be able to write to a class A file when locked"
		);

	T_ASSERT_EQ(
		SET_PROT_CLASS(g_fd, PROTECTION_CLASS_D),
		-1,
		"Should not be able to change protection "
		"from class A to class D when locked"
		);

	                                        
	close(g_fd);

	T_ASSERT_EQ(
		g_fd = open(g_filepath, O_RDWR | O_TRUNC | O_CLOEXEC),
		-1,
		"Should not be able to open and truncate "
		"a class A file when locked"
		);

	                          
	T_ASSERT_EQ(
		g_fd = open(g_filepath, O_RDWR | O_CLOEXEC),
		-1,
		"Should not be able to open a class A file when locked"
		);

	                               
	T_ASSERT_EQ(unlock_device(TEST_PASSCODE), 0, "*** Unlock device ***");

	T_ASSERT_GE(
		g_fd = open(g_filepath, O_RDWR | O_CLOEXEC),
		0,
		"Should be able to open a class A file when unlocked"
		);

	T_WITH_ERRNO;
	T_ASSERT_EQ(
		SET_PROT_CLASS(g_fd, PROTECTION_CLASS_D),
		0,
		"Should be able to change protection "
		"class from A to D when unlocked"
		);

	T_ASSERT_EQ(lock_device(), 0, "*** Lock device ***");

	                                                
	T_ASSERT_EQ(
		SET_PROT_CLASS(g_fd, PROTECTION_CLASS_B),
		0,
		"Should be able to change protection "
		"class from D to B when locked"
		);

	T_ASSERT_EQ(
		GET_PROT_CLASS(g_fd),
		PROTECTION_CLASS_B,
		"File should now have class B protection"
		);

	  
                                               
                                     
    
	current_byte = 0;

	while (current_byte < CPT_IO_SIZE) {
		local_result = pwrite(
			g_fd,
			&wr_buffer[current_byte],
			CPT_IO_SIZE - current_byte,
			current_byte
			);

		T_WITH_ERRNO;
		T_ASSERT_NE(
			local_result,
			-1,
			"Should be able to write to a "
			"new class B file when locked"
			);

		current_byte += local_result;
	}

	current_byte = 0;

	while (current_byte < CPT_IO_SIZE) {
		local_result = pread(
			g_fd,
			&rd_buffer[current_byte],
			CPT_IO_SIZE - current_byte,
			current_byte
			);

		T_ASSERT_NE(
			local_result,
			-1,
			"Should be able to read from a "
			"new class B file when locked"
			);

		current_byte += local_result;
	}

	                                                              
	close(g_fd);
	T_WITH_ERRNO;
	T_ASSERT_EQ(
		g_fd = open(g_filepath, O_RDWR | O_CLOEXEC),
		-1,
		"Should not be able to open a class B file when locked"
		);

	unlink(g_filepath);

	                                                
	T_WITH_ERRNO;
	T_ASSERT_NE(
		mkdir(g_dirpath, 0x0777),
		-1,
		"Should be able to create a new directory when locked"
		);

	                                                                     
	T_ASSERT_NE(
		g_dir_fd = open(g_dirpath, O_RDONLY | O_CLOEXEC),
		-1,
		"Should be able to open an unclassed directory when locked"
		);

	T_ASSERT_TRUE(
		GET_PROT_CLASS(g_dir_fd) == PROTECTION_CLASS_D ||
		GET_PROT_CLASS(g_dir_fd) == PROTECTION_CLASS_DIR_NONE,
		"Directory protection class sholud be D or NONE"
		);

	T_ASSERT_EQ(
		SET_PROT_CLASS(g_dir_fd, PROTECTION_CLASS_A),
		0,
		"Should be able to change a directory from "
		"class D to class A while locked"
		);

	T_ASSERT_EQ(
		SET_PROT_CLASS(g_dir_fd, PROTECTION_CLASS_D),
		0,
		"Should be able to change a directory from "
		"class A to class D while locked"
		);

	  
                                                              
                                 
    
	T_SETUPBEGIN;
	T_ASSERT_LT(
		strlcpy(g_filepath, g_dirpath, PATH_MAX),
		PATH_MAX,
		"Construct path for file in the directory"
		);
	T_ASSERT_LT(
		strlcat(g_filepath, "test_file", PATH_MAX),
		PATH_MAX,
		"Construct path for file in the directory"
		);
	T_SETUPEND;

	T_ASSERT_EQ(unlock_device(TEST_PASSCODE), 0, "*** Unlock device ***");

	for (
		new_prot_class = PROTECTION_CLASS_A;
		new_prot_class <= PROTECTION_CLASS_D;
		new_prot_class++
		) {
		int getclass_dir;

		T_WITH_ERRNO;
		T_ASSERT_NE(
			old_prot_class = GET_PROT_CLASS(g_dir_fd),
			-1,
			"Get protection class for the directory"
			);

		T_WITH_ERRNO;
		T_ASSERT_EQ(
			SET_PROT_CLASS(g_dir_fd, new_prot_class),
			0,
			"Should be able to change directory "
			"protection from %s to %s",
			dp_class_num_to_string(old_prot_class),
			dp_class_num_to_string(new_prot_class)
			);

		T_EXPECT_EQ(
			getclass_dir = GET_PROT_CLASS(g_dir_fd),
			new_prot_class,
			"Get protection class for the directory"
			);

		T_WITH_ERRNO;
		T_ASSERT_GE(
			g_fd = open(g_filepath, O_CREAT | O_EXCL | O_CLOEXEC, 0777),
			0,
			"Should be able to create file in "
			"%s directory when unlocked",
			dp_class_num_to_string(new_prot_class)
			);

		T_WITH_ERRNO;
		T_ASSERT_NE(
			local_result = GET_PROT_CLASS(g_fd),
			-1,
			"Get the new file's protection class"
			);

		T_ASSERT_EQ(
			local_result,
			new_prot_class,
			"File should have %s protection",
			dp_class_num_to_string(new_prot_class)
			);

		close(g_fd);
		unlink(g_filepath);
	}

	                                                     
	T_ASSERT_NE(
		SET_PROT_CLASS(g_dir_fd, PROTECTION_CLASS_F),
		0,
		"Should not be able to create class F directory"
		);

	  
                                                       
                                      
    
	T_WITH_ERRNO;
	T_ASSERT_EQ(
		SET_PROT_CLASS(g_dir_fd, PROTECTION_CLASS_A),
		0,
		"Should be able to change protection "
		"from class F to class A when unlocked"
		);

	T_ASSERT_EQ(lock_device(), 0, "*** Lock device ***");

	T_ASSERT_EQ(
		g_fd = open(g_filepath, O_CREAT | O_EXCL | O_CLOEXEC, 0777),
		-1,
		"Should not be able to create a new file "
		"in a class A directory when locked"
		);

	T_ASSERT_EQ(unlock_device(TEST_PASSCODE), 0, "*** Unlock device ***");

	T_WITH_ERRNO;
	T_ASSERT_EQ(
		SET_PROT_CLASS(g_dir_fd, PROTECTION_CLASS_B),
		0,
		"Should be able to change directory "
		"from class A to class B when unlocked"
		);

	T_ASSERT_EQ(lock_device(), 0, "*** Lock device ***");

	T_ASSERT_GE(
		g_fd = open(g_filepath, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0777),
		0,
		"Should be able to create a new file "
		"in class B directory when locked"
		);

	T_ASSERT_NE(
		local_result = GET_PROT_CLASS(g_fd),
		-1,
		"Get the new file's protection class"
		);

	T_ASSERT_EQ(
		local_result,
		PROTECTION_CLASS_B,
		"File should inherit protection class of class B directory"
		);

	                                                            
	T_ASSERT_EQ(unlock_device(TEST_PASSCODE), 0, "*** Unlock device ***");

	for (
		new_prot_class = PROTECTION_CLASS_A;
		new_prot_class <= PROTECTION_CLASS_D;
		new_prot_class++
		) {
		T_WITH_ERRNO;
		T_ASSERT_EQ(
			SET_PROT_CLASS(g_dir_fd, new_prot_class),
			0,
			"Change directory to %s",
			dp_class_num_to_string(new_prot_class)
			);

		T_WITH_ERRNO;
		T_ASSERT_NE(
			mkdir(g_subdirpath, 0x0777),
			-1,
			"Create subdirectory in %s directory",
			dp_class_num_to_string(new_prot_class)
			);

		T_WITH_ERRNO;
		T_ASSERT_NE(
			g_subdir_fd = open(g_subdirpath, O_RDONLY | O_CLOEXEC),
			-1,
			"Should be able to open subdirectory in %s directory",
			dp_class_num_to_string(new_prot_class)
			);

		T_ASSERT_NE(
			local_result = GET_PROT_CLASS(g_subdir_fd),
			-1,
			"Get protection class of new subdirectory "
			"of %s directory",
			dp_class_num_to_string(new_prot_class)
			);

		T_ASSERT_EQ(
			local_result,
			new_prot_class,
			"New subdirectory should have same class as %s parent",
			dp_class_num_to_string(new_prot_class)
			);

		close(g_subdir_fd);
		rmdir(g_subdirpath);
	}
}

void
setup(void)
{
	int ret = 0;
	int local_result = -1;

	T_SETUPBEGIN;

	T_ATEND(cleanup);

	T_WITH_ERRNO;
	T_ASSERT_NOTNULL(
		mkdtemp(g_test_tempdir),
		"Create temporary directory for test"
		);
	T_LOG("Test temp dir: %s", g_test_tempdir);

	T_ASSERT_NE(
		local_result = supports_content_prot(),
		-1,
		"Get content protection support status"
		);

	if (local_result == 0) {
		T_SKIP("Data protection not supported on this system");
	}

	T_ASSERT_EQ(
		has_passcode(),
		0,
		"Device should not have existing passcode"
		);

	T_ASSERT_EQ(
		set_passcode(TEST_PASSCODE, NULL),
		0,
		"Set test passcode"
		);

	bzero(g_filepath, PATH_MAX);
	bzero(g_dirpath, PATH_MAX);
	bzero(g_subdirpath, PATH_MAX);

	ret |= (strlcat(g_filepath, g_test_tempdir, PATH_MAX) == PATH_MAX);
	ret |= (strlcat(g_filepath, "/", PATH_MAX) == PATH_MAX);
	ret |= (strlcpy(g_dirpath, g_filepath, PATH_MAX) == PATH_MAX);
	ret |= (strlcat(g_filepath, "test_file", PATH_MAX) == PATH_MAX);
	ret |= (strlcat(g_dirpath, "test_dir/", PATH_MAX) == PATH_MAX);
	ret |= (strlcpy(g_subdirpath, g_dirpath, PATH_MAX) == PATH_MAX);
	ret |= (strlcat(g_subdirpath, "test_subdir/", PATH_MAX) == PATH_MAX);

	T_QUIET;
	T_ASSERT_EQ(ret, 0, "Initialize test path strings");

	T_WITH_ERRNO;
	T_ASSERT_GE(
		g_fd = open(g_filepath, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0777),
		0,
		"Create test file"
		);

	T_SETUPEND;
}

void
cleanup(void)
{
	T_LOG("Cleaning up…");

	if (g_subdir_fd >= 0) {
		T_LOG("Cleanup: closing fd %d", g_subdir_fd);
		close(g_subdir_fd);
	}

	if (g_subdirpath[0]) {
		T_LOG("Cleanup: removing %s", g_subdirpath);
		rmdir(g_subdirpath);
	}

	if (g_fd >= 0) {
		T_LOG("Cleanup: closing fd %d", g_fd);
		close(g_fd);
	}

	if (g_filepath[0]) {
		T_LOG("Cleanup: removing %s", g_filepath);
		unlink(g_filepath);
	}

	if (g_dir_fd >= 0) {
		T_LOG("Cleanup: closing fd %d", g_dir_fd);
		close(g_dir_fd);
	}

	if (g_dirpath[0]) {
		T_LOG("Cleanup: removing %s", g_dirpath);
		rmdir(g_dirpath);
	}

	if (strcmp(g_test_tempdir, TEMP_DIR_TEMPLATE)) {
		T_LOG("Cleanup: removing %s", g_test_tempdir);
		rmdir(g_test_tempdir);
	}

	if (g_passcode_set) {
		T_LOG("Cleanup: unlocking device");
		if (unlock_device(TEST_PASSCODE)) {
			T_LOG("Warning: failed to unlock device in cleanup");
		}

		T_LOG("Cleanup: clearing passcode");
		if (clear_passcode(TEST_PASSCODE)) {
			T_LOG("Warning: failed to clear passcode in cleanup");
		}
	}
}

int
set_passcode(char * new_passcode, char * old_passcode)
{
	int result = -1;

#ifdef KEYBAG_ENTITLEMENTS
	                                                           
	uint64_t inputs[] = {device_keybag_handle};
	uint32_t input_count = (sizeof(inputs) / sizeof(*inputs));
	void * input_structs = NULL;
	size_t input_struct_count = 0;
	char buffer[CPT_AKS_BUF_SIZE];
	char * buffer_ptr = buffer;
	uint32_t old_passcode_len = 0;
	uint32_t new_passcode_len = 0;

	T_LOG("%s(): using keybag entitlements", __func__);

	old_passcode_len = strnlen(old_passcode, CPT_MAX_PASS_LEN);
	new_passcode_len = strnlen(new_passcode, CPT_MAX_PASS_LEN);

	if ((old_passcode == NULL) || (old_passcode_len == CPT_MAX_PASS_LEN)) {
		old_passcode = "";
		old_passcode_len = 0;
	}
	if ((new_passcode == NULL) || (new_passcode_len == CPT_MAX_PASS_LEN)) {
		new_passcode = "";
		new_passcode_len = 0;
	}

	*((uint32_t *) buffer_ptr) = ((uint32_t) 2);
	buffer_ptr += sizeof(uint32_t);

	*((uint32_t *) buffer_ptr) = old_passcode_len;
	buffer_ptr += sizeof(uint32_t);

	memcpy(buffer_ptr, old_passcode, old_passcode_len);
	buffer_ptr += ((old_passcode_len + sizeof(uint32_t) - 1) &
	    ~(sizeof(uint32_t) - 1));

	*((uint32_t *) buffer_ptr) = new_passcode_len;
	buffer_ptr += sizeof(uint32_t);

	memcpy(buffer_ptr, new_passcode, new_passcode_len);
	buffer_ptr += ((new_passcode_len + sizeof(uint32_t) - 1) &
	    ~(sizeof(uint32_t) - 1));

	input_structs = buffer;
	input_struct_count = (buffer_ptr - buffer);

	result = apple_key_store(
		kAppleKeyStoreKeyBagSetPasscode,
		inputs,
		input_count,
		input_structs,
		input_struct_count,
		NULL,
		NULL
		);
#else
	  
                                            
                                    
    
	T_LOG("%s(): using keystorectl", __func__);

	if (
		(old_passcode == NULL) ||
		(strnlen(old_passcode, CPT_MAX_PASS_LEN) == CPT_MAX_PASS_LEN)
		) {
		old_passcode = "";
	}

	if (
		(new_passcode == NULL) ||
		(strnlen(new_passcode, CPT_MAX_PASS_LEN) == CPT_MAX_PASS_LEN)
		) {
		new_passcode = "";
	}

	char * const keystorectl_args[] = {
		KEYBAGDTEST_PATH,
		"syspass",
		old_passcode,
		new_passcode,
		NULL
	};
	result = spawn_proc(keystorectl_args);
#endif                          
	if (result == 0 && new_passcode != NULL) {
		g_passcode_set = 1;
	} else if (result == 0 && new_passcode == NULL) {
		g_passcode_set = 0;
	}

	return result;
}

int
clear_passcode(char * passcode)
{
	  
                                                                  
                                                                     
                                  
    
	return set_passcode(NULL, passcode);
}

int
has_passcode(void)
{
	return set_passcode(NULL, NULL);
}

int
lock_device(void)
{
	int result = -1;

	  
                                                                     
                                                                      
                                   
   
                                                                     
                                                                    
                                                                     
                                                                     
                    
    
	char * const kbd_args[] = {KEYBAGDTEST_PATH, "lock", NULL};
	result = spawn_proc(kbd_args);
	if (result) {
		return result;
	}

	  
                                                                    
                                                                    
                        
    
	(void) unlink("/private/var/foo_test_file");

	while (1) {
		int dp_fd;

		dp_fd = open_dprotected_np(
			"/private/var/foo_test_file",
			O_RDWR | O_CREAT,
			PROTECTION_CLASS_A,
			0
			);

		if (dp_fd >= 0) {
			                         
			close(dp_fd);
			result = unlink("/private/var/foo_test_file");

			if (result) {
				return result;
			}

			sync();
			sleep(1);
		} else {
			                                   
			break;
		}
	}

	  
                                                                  
                                                                  
                                                                     
                                                    
    

	return 0;
}

int
unlock_device(char * passcode)
{
	int result = -1;

#ifdef  KEYBAG_ENTITLEMENTS
	                                                            
	uint64_t inputs[] = {device_keybag_handle};
	uint32_t input_count = (sizeof(inputs) / sizeof(*inputs));
	size_t input_struct_count = 0;

	T_LOG("%s(): using keybag entitlements", __func__);

	input_struct_count = strnlen(passcode, CPT_MAX_PASS_LEN);
	if ((passcode == NULL) || (input_struct_count == CPT_MAX_PASS_LEN)) {
		passcode = "";
		input_struct_count = 0;
	}

	result = apple_key_store(
		kAppleKeyStoreKeyBagUnlock,
		inputs,
		input_count,
		passcode,
		input_struct_count,
		NULL,
		NULL
		);
#else
	  
                                            
                                     
    
	T_LOG("%s(): using keystorectl", __func__);

	if (
		(passcode == NULL) ||
		(strnlen(passcode, CPT_MAX_PASS_LEN) == CPT_MAX_PASS_LEN)
		) {
		passcode = "";
	}

	char * const keystorectl_args[] = {
		KEYSTORECTL_PATH, "unlock", passcode, NULL
	};

	result = spawn_proc(keystorectl_args);
#endif                          

	return result;
}

  
                                             
                                         
                                          
  
                                                                 
                           
   
int
supports_content_prot(void)
{
	int local_result = -1;
	int result = -1;
	uint32_t buffer_size = 1;
	char buffer[buffer_size];
	io_registry_entry_t defaults = IO_OBJECT_NULL;
	kern_return_t k_result = KERN_FAILURE;
	struct statfs statfs_results;

	defaults = IORegistryEntryFromPath(
		kIOMasterPortDefault,
		kIODeviceTreePlane ":/defaults"
		);

	if (defaults == IO_OBJECT_NULL) {
		                                           
		T_LOG(
			"%s(): no defaults entry in IORegistry",
			__func__
			);
		return 0;
	}

	k_result = IORegistryEntryGetProperty(
		defaults,
		"content-protect",
		buffer,
		&buffer_size
		);

	if (k_result != KERN_SUCCESS) {
		                                           
		T_LOG(
			"%s(): no content-protect property in IORegistry",
			__func__
			);
		return 0;
	}

	  
                                                            
                                                                 
                                               
    
	local_result = statfs(g_test_tempdir, &statfs_results);

	if (local_result == -1) {
		T_LOG(
			"%s(): failed to statfs the test directory, errno = %s",
			__func__, strerror(errno)
			);
		return -1;
	} else if (statfs_results.f_flags & MNT_CPROTECT) {
		return 1;
	} else {
		T_LOG(
			"%s(): filesystem not formatted for data protection",
			__func__
			);
		return 0;
	}
}

  
                                                
                                              
   
int
apple_key_store(uint32_t command,
    uint64_t * inputs,
    uint32_t input_count,
    void * input_structs,
    size_t input_struct_count,
    uint64_t * outputs,
    uint32_t * output_count)
{
	int result = -1;
	io_connect_t connection = IO_OBJECT_NULL;
	io_registry_entry_t apple_key_bag_service = IO_OBJECT_NULL;
	kern_return_t k_result = KERN_FAILURE;
	IOReturn io_result = IO_OBJECT_NULL;

	apple_key_bag_service = IOServiceGetMatchingService(
		kIOMasterPortDefault,
		IOServiceMatching(kAppleKeyStoreServiceName)
		);
	if (apple_key_bag_service == IO_OBJECT_NULL) {
		T_LOG(
			"%s: failed to match kAppleKeyStoreServiceName",
			__func__
			);
		goto end;
	}

	k_result = IOServiceOpen(
		apple_key_bag_service,
		mach_task_self(),
		0,
		&connection
		);
	if (k_result != KERN_SUCCESS) {
		T_LOG(
			"%s: failed to open AppleKeyStore: "
			"IOServiceOpen() returned %d",
			__func__, k_result
			);
		goto end;
	}

	k_result = IOConnectCallMethod(
		connection,
		kAppleKeyStoreUserClientOpen,
		NULL, 0, NULL, 0, NULL, NULL, NULL, NULL
		);
	if (k_result != KERN_SUCCESS) {
		T_LOG(
			"%s: call to AppleKeyStore method "
			"kAppleKeyStoreUserClientOpen failed",
			__func__
			);
		goto close;
	}

	io_result = IOConnectCallMethod(
		connection, command, inputs, input_count, input_structs,
		input_struct_count, outputs, output_count, NULL, NULL
		);
	if (io_result != kIOReturnSuccess) {
		T_LOG("%s: call to AppleKeyStore method %d failed", __func__, command);
		goto close;
	}

	result = 0;

close:
	IOServiceClose(apple_key_bag_service);
end:
	return result;
}

  
                                      
   
int
spawn_proc(char * const command[])
{
	pid_t pid           = 0;
	int launch_tool_ret = 0;
	bool waitpid_ret    = true;
	int status          = 0;
	int signal          = 0;
	int timeout         = 30;

	launch_tool_ret = dt_launch_tool(&pid, command, false, NULL, NULL);
	T_EXPECT_EQ(launch_tool_ret, 0, "launch tool: %s", command[0]);
	if (launch_tool_ret != 0) {
		return 1;
	}

	waitpid_ret = dt_waitpid(pid, &status, &signal, timeout);
	T_EXPECT_TRUE(waitpid_ret, "%s should succeed", command[0]);
	if (waitpid_ret == false) {
		if (status != 0) {
			T_LOG("%s exited %d", command[0], status);
		}
		if (signal != 0) {
			T_LOG("%s received signal %d", command[0], signal);
		}
		return 1;
	}

	return 0;
}

char*
dp_class_num_to_string(int num)
{
	switch (num) {
	case 0:
		return "unclassed";
	case PROTECTION_CLASS_A:
		return "class A";
	case PROTECTION_CLASS_B:
		return "class B";
	case PROTECTION_CLASS_C:
		return "class C";
	case PROTECTION_CLASS_D:
		return "class D";
	case PROTECTION_CLASS_E:
		return "class E";
	case PROTECTION_CLASS_F:
		return "class F";
	default:
		return "<unknown class>";
	}
}

#if 0
int
device_lock_state(void)
{
	  
                                  
   
                                                                    
                                                                    
                             
    
	int result = -1;

	return result;
}

                                                           
int
unlocked_since_boot()
{
	  
                                  
   
                                                                   
                                                                 
                                                             
                          
    
	int result = 1;

	return result;
}
#endif
