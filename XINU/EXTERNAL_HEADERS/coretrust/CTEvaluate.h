  
               
             
  
//  Copyright © 2017-2020 Apple Inc. All rights reserved.
  

#ifndef _CORETRUST_EVALUATE_H_
#define _CORETRUST_EVALUATE_H_

#include <stdint.h>
#include <stdbool.h>

__BEGIN_DECLS

typedef struct x509_octet_string {
    const uint8_t *data;
    size_t length;
} CTAsn1Item;

int CTParseCertificateSet(const uint8_t *der, const uint8_t *der_end,                                                                        
                          CTAsn1Item *certStorage, size_t certStorageLen,                                                                                    
                                                                                                                                              
                          size_t *numParsedCerts);                                                                        

int CTEvaluateSavageCerts(const uint8_t *certsData, size_t certsLen,
                          const uint8_t *rootKeyData, size_t rootKeyLen,
                          const uint8_t **leafKeyData, size_t *leafKeyLen,
                          bool *isProdCert);

int CTEvaluateSavageCertsWithUID(const uint8_t *certsData, size_t certsLen,
                                 const uint8_t *rootKeyData, size_t rootKeyLen,
                                 const uint8_t **leafKeyData, size_t *leafKeyLen,                                                              
                                 uint8_t *UIDData, size_t UIDLen,                                                            
                                 bool *isProdCert);

int CTEvaluateYonkersCerts(const uint8_t *certsData, size_t certsLen,
                           const uint8_t *rootKeyData, size_t rootKeyLen,
                           const uint8_t **leafKeyData, size_t *leafKeyLen,                                                              
                           uint8_t *UIDData, size_t UIDLen,                                                            
                           bool *isProdCert);

int CTEvaluateAcrt(const uint8_t *certsData, size_t certsLen,                                                                        
                                                                                                                      
                   const uint8_t **leafKeyData, size_t *leafKeyLen);                                                               

int CTEvaluateUcrt(const uint8_t *certsData, size_t certsLen,                                                                  
                                                                                                                  
                   const uint8_t **leafKeyData, size_t *leafKeyLen);                                                                

int CTEvaluateUcrtTestRoot(const uint8_t *certsData, size_t certsLen,                                                                  
                                                                                                                          
                           const uint8_t *rootKeyData, size_t rootKeyLen,                                                                             
                           const uint8_t **leafKeyData, size_t *leafKeyLen);                                                                

int CTEvaluateBAASystem(const uint8_t *certsData, size_t certsLen,                                                                  
                                                                                                                       
                        const uint8_t **leafKeyData, size_t *leafKeyLen);                                                               

typedef struct baa_identity {
    uint32_t chipId;
    uint64_t ecid;
    bool productionStatus;
    bool securityMode;
    uint8_t securityDomain;
    CTAsn1Item img4;
} CTBAAIdentity;

int CTEvaluateBAASystemWithId(const uint8_t *certsData, size_t certsLen,                                                                 
                                                                                                                            
                              const uint8_t **leafKeyData, size_t *leafKeyLen,                                                               
                              CTBAAIdentity *identity);                                                                          

int CTEvaluateBAASystemTestRoot(const uint8_t *certsData, size_t certsLen,                                                               
                                                                                                                            
                                const uint8_t *rootKeyData, size_t rootKeyLen,                                                                          
                                const uint8_t **leafKeyData, size_t *leafKeyLen,                                                             
                                CTBAAIdentity *identity);                                                                        

int CTEvaluateBAAUser(const uint8_t *certsData, size_t certsLen,                                                                 
                                                                                                                    
                      const uint8_t **leafKeyData, size_t *leafKeyLen,                                                               
                      CTBAAIdentity *identity);                                                                          

int CTEvaluateBAAUserTestRoot(const uint8_t *certsData, size_t certsLen,                                                                 
                                                                                                                            
                              const uint8_t *rootKeyData, size_t rootKeyLen,                                                                            
                              const uint8_t **leafKeyData, size_t *leafKeyLen,                                                               
                              CTBAAIdentity *identity);                                                                          

int CTEvaluateSatori(const uint8_t *certsData, size_t certsLen,                                                                      
                                                                                                 
                     bool allowTestRoot,                                                                               
                     const uint8_t **leafKeyData, size_t *leafKeyLen);                                                               

int CTEvaluatePragueSignatureCMS(const uint8_t *cmsData, size_t cmsLen,                                             
                                 const uint8_t *detachedData, size_t detachedDataLen,                                    
                                 bool allowTestRoot,                                                                          
                                 const uint8_t **leafKeyData, size_t *leafKeyLen);                                                         

int CTEvaluateKDLSignatureCMS(const uint8_t *cmsData, size_t cmsLen,                                                
                              const uint8_t *detachedData, size_t detachedDataLen,                                       
                              bool allowTestRoot,                                                                             
                              const uint8_t **leafKeyData, size_t *leafKeyLen);                                                            

typedef uint64_t CoreTrustPolicyFlags;
enum {
    CORETRUST_POLICY_BASIC =                0,
    CORETRUST_POLICY_SAVAGE_DEV =           1 << 0,
    CORETRUST_POLICY_SAVAGE_PROD =          1 << 1,
    CORETRUST_POLICY_MFI_AUTHV3 =           1 << 2,
    CORETRUST_POLICY_MAC_PLATFORM =         1 << 3,
    CORETRUST_POLICY_MAC_DEVELOPER =        1 << 4,
    CORETRUST_POLICY_DEVELOPER_ID =         1 << 5,
    CORETRUST_POLICY_MAC_APP_STORE =        1 << 6,
    CORETRUST_POLICY_IPHONE_DEVELOPER =     1 << 7,
    CORETRUST_POLICY_IPHONE_APP_PROD =      1 << 8,
    CORETRUST_POLICY_IPHONE_APP_DEV =       1 << 9,
    CORETRUST_POLICY_IPHONE_VPN_PROD =      1 << 10,
    CORETRUST_POLICY_IPHONE_VPN_DEV =       1 << 11,
    CORETRUST_POLICY_TVOS_APP_PROD =        1 << 12,
    CORETRUST_POLICY_TVOS_APP_DEV =         1 << 13,
    CORETRUST_POLICY_TEST_FLIGHT_PROD =     1 << 14,
    CORETRUST_POLICY_TEST_FLIGHT_DEV =      1 << 15,
    CORETRUST_POLICY_IPHONE_DISTRIBUTION =  1 << 16,
    CORETRUST_POLICY_MAC_SUBMISSION =       1 << 17,
    CORETRUST_POLICY_YONKERS_DEV =          1 << 18,
    CORETRUST_POLICY_YONKERS_PROD =         1 << 19,
    CORETRUST_POLICY_MAC_PLATFORM_G2 =      1 << 20,
    CORETRUST_POLICY_ACRT =                 1 << 21,
    CORETRUST_POLICY_SATORI =               1 << 22,
    CORETRUST_POLICY_BAA =                  1 << 23,
    CORETRUST_POLICY_UCRT =                 1 << 24,
    CORETRUST_POLICY_PRAGUE =               1 << 25,
    CORETRUST_POLICY_KDL =                  1 << 26,
    CORETRUST_POLICY_MFI_AUTHV2 =           1 << 27,
    CORETRUST_POLICY_MFI_SW_AUTH_PROD =     1 << 28,
    CORETRUST_POLICY_MFI_SW_AUTH_DEV =      1 << 29,
    CORETRUST_POLICY_COMPONENT =            1 << 30,
    CORETRUST_POLICY_IMG4 =                 1ULL << 31,
    CORETRUST_POLICY_SERVER_AUTH =          1ULL << 32,
    CORETRUST_POLICY_SERVER_AUTH_STRING =   1ULL << 33,
};

typedef uint32_t CoreTrustDigestType;
enum {
    CORETRUST_DIGEST_TYPE_SHA1 = 1,
    CORETRUST_DIGEST_TYPE_SHA224 = 2,
    CORETRUST_DIGEST_TYPE_SHA256 = 4,
    CORETRUST_DIGEST_TYPE_SHA384 = 8,
    CORETRUST_DIGEST_TYPE_SHA512 = 16
};

int CTEvaluateAMFICodeSignatureCMS(const uint8_t *cmsData, size_t cmsLen,                                     
                                   const uint8_t *detachedData, size_t detachedDataLen,                                      
                                   bool allow_test_hierarchy,                                                                     
                                   const uint8_t **leafCert, size_t *leafCertLen,                                         
                                   CoreTrustPolicyFlags *policyFlags,                                                                   
                                   CoreTrustDigestType *cmsDigestType,                                                                 
                                   CoreTrustDigestType *hashAgilityDigestType,                                                    
                                                                                                                                   
                                   const uint8_t **digestData, size_t *digestLen);                                                  
                                                                                                                                            
                                                                                      
                                                                                                                       
                                                                                                                                        
                                         
                                                              
                                                                                    
                                                                                                                       
                                                                                                      
   

int CTParseAccessoryCerts(const uint8_t *certsData, size_t certsLen,                                                                               
                                  const uint8_t **leafCertData, size_t *leafCertLen,                                                          
                                  const uint8_t **subCACertData, size_t *subCACertLen,                                                                                                                                   
                                  CoreTrustPolicyFlags *flags);                                                                 


int CTEvaluateAccessoryCert(const uint8_t *leafCertData, size_t leafCertLen,                                                                    
                            const uint8_t *subCACertData, size_t subCACertLen,                                                                              
                            const uint8_t *anchorCertData, size_t anchorCertLen,                                                                  
                            CoreTrustPolicyFlags policy,                                                                             
                            const uint8_t **leafKeyData, size_t *leafKeyLen,                                                                            
                            const uint8_t **extensionValueData, size_t *extensionValueLen);                                                                   
                                                                                            
                                                                                         
                                                                                                                        
                                                                                                               
  
                                                   
                            
                                 
                                 
                                      
                                       
                                
   

int CTEvaluateAppleSSL(const uint8_t *certsData, size_t certsLen,                                                                  
                                                                                                                               
                       const uint8_t *hostnameData, size_t hostnameLen,                                                                
                       uint64_t leafMarker,                                                                                              
                                                                                                                             
                       bool allowTestRoots);                                                                      

int CTEvaluateAppleSSLWithOptionalTemporalCheck(const uint8_t *certsData, size_t certsLen,
                                                 const uint8_t *hostnameData, size_t hostnameLen,
                                                 uint64_t leafMarker,
                                                 bool allowTestRoots,
                                                 bool checkTemporalValidity);

__END_DECLS

#endif                             
