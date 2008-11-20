typedef struct {
  unsigned long ek[32]; // encryption key
  unsigned long dk[32]; // decryption key
} des_ctx;

void des_key(des_ctx *dc, unsigned char key[8]);
void cbc_des_enc(des_ctx *dc, unsigned char *IV, unsigned char *Data, int blocks);
void cbc_des_dec(des_ctx *dc, unsigned char *IV, unsigned char *Data, int blocks);