/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _NET_MULTI_LAYER_PKT_LOG_H_
#define _NET_MULTI_LAYER_PKT_LOG_H_

#include <os/log.h>

  
                                                             
   
#define MPKL_VERSION 1

  
                                                                                
                                                                                                         
                                                                
  
   

#define MPKL_PROTOCOL_PHYSICAL               ((uint8_t)0)                                     

#define MPKL_PROTOCOL_BT                     ((uint8_t)20)
#define MPKL_PROTOCOL_WIFI                   ((uint8_t)30)
#define MPKL_PROTOCOL_CELLULAR               ((uint8_t)40)
#define MPKL_PROTOCOL_TERMINUS               ((uint8_t)60)
#define MPKL_PROTOCOL_IPSEC                  ((uint8_t)80)
#define MPKL_PROTOCOL_TCP                    ((uint8_t)100)
#define MPKL_PROTOCOL_IDS                    ((uint8_t)120)
#define MPKL_PROTOCOL_LIBNETCORE             ((uint8_t)140)
#define MPKL_PROTOCOL_CFNETWORK              ((uint8_t)160)
#define MPKL_PROTOCOL_REMOTE_CONNECTION      ((uint8_t)200)

#define MPKL_TOPMOST_LAYER                   ((uint8_t)255)                          


   
                                
                                                                                                     
  
                                                   
  
                                  
  
   
#define MPKL_CREATE_LOGOBJECT(Name)    os_log_create("com.apple.magnetpacketlog", Name)

  
                               
  
   

   
                                        
                                                                                                                                
  
                                                                     
                                                                                                    
                                                                                 
                                                                                
                                                                                 
                                                                       
                                                                                                                  
   

#define MPKL_UUID_UUID_ASSOCIATE_PREV(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, CUR_UUID, PREV_UUID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "1 {curProtocol: %hhu, prevProtocol: %hhu, curUUID: %{public,uuid_t}.16P, prevUUID: %{public,uuid_t}.16P, curPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, CUR_UUID, PREV_UUID, CUR_LEN, LOG_SEQN)

   
                                        
                                                                                                                              
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                             
                                                                                
                                                                             
                                                                       
                                                                                                                  
   

#define MPKL_UUID_UUID_ASSOCIATE_NEXT(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, NEXT_UUID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "2 {curProtocol: %hhu, nextProtocol: %hhu, curUUID: %{public,uuid_t}.16P, nextUUID: %{public,uuid_t}.16P, curPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, NEXT_UUID, CUR_LEN, LOG_SEQN)

   
                                       
                                                                                                            
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                             
                                                                                   
                                                                                 
                                                                            
                                                                       
                                                                                                                  
   

#define MPKL_SEQRANGE_UUID_ASSOCIATE(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_RANGE_START, PREV_RANGE_END, CUR_UUID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "3 {curProtocol: %hhu, prevProtocol: %hhu, prevStart: %u, prevEnd: %u, curUUID: %{public,uuid_t}.16P, curPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_RANGE_START, PREV_RANGE_END, CUR_UUID, CUR_LEN, LOG_SEQN)

   
                                       
                                                                                                            
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                             
                                                                                 
                                                                                   
                                                                                 
                                                                        
                                                                                                                  
   

#define MPKL_UUID_SEQRANGE_ASSOCIATE(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_UUID, CUR_RANGE_START, CUR_RANGE_END, PREV_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "4 {curProtocol: %hhu, prevProtocol: %hhu, prevUUID: %{public,uuid_t}.16P, curStart: %u, curEnd: %u, prevPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_UUID, CUR_RANGE_START, CUR_RANGE_END, PREV_LEN, LOG_SEQN)


   
                                       
                                                                                     
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                              
                                                                      
                                                                                
                                                                       
                                                                                                                  
   

#define MPKL_BUNDLEID_UUID_ASSOCIATE(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_BUNDLE_ID, CUR_UUID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "5 {curProtocol: %hhu, prevProtocol: %hhu, prevBundleID: %@, curUUID: %{public,uuid_t}.16P, curPktLen: %u, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_BUNDLE_ID, CUR_UUID, CUR_LEN, LOG_SEQN)


   
                                                  
                                                                                                                          
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                              
                                                                                   
                                                                                 
                                                                                
                                                                      
                                                                       
                                                                                                                  
   

#define MPKL_SEQRANGE_UUID_ASSOCIATE_W_BUNDLEID(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_RANGE_START, PREV_RANGE_END, CUR_UUID, CLIENT_BUNDLE_ID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "6 {curProtocol: %hhu, prevProtocol: %hhu, prevStart: %u, prevEnd: %u, curUUID: %{public,uuid_t}.16P, curBundleID: %@, curPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_RANGE_START, PREV_RANGE_END, CUR_UUID, CLIENT_BUNDLE_ID, CUR_LEN, LOG_SEQN)


   
                                        
                                                                                                                  
                                                                                                                                               
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                             
                                                                              
                                                                                  
                                                                                
                                                                        
                                                                        
                                                                                   
                                                                                 
                                                                                                                  
   

#define MPKL_SEQN_UUID_ASSOCIATE_PREV(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_UUID, PREV_RANGE_START, PREV_RANGE_END, PREV_LEN, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, LOG_SEQN) \
os_log(LOGOBJECT, "7 {Send, curProtocol: %hhu, prevProtocol: %hhu, prevUUID: %{public,uuid_t}.16P, prevStart: %hu, prevEnd: %hu, prevPktLen %hu, curSeqN: %hu, curStart: %hu, curEnd: %hu,  logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_UUID, PREV_RANGE_START, PREV_RANGE_END, PREV_LEN, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, LOG_SEQN)

   
                                        
                                                                                                                  
                                                                                                                                               
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                             
                                                                              
                                                                                  
                                                                                
                                                                       
                                                                        
                                                                                   
                                                                                 
                                                                                                                  
   

#define MPKL_SEQN_UUID_ASSOCIATE_NEXT(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, NEXT_UUID, NEXT_RANGE_START, NEXT_RANGE_END, NEXT_LEN, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, LOG_SEQN) \
os_log(LOGOBJECT, "8 {Receive, curProtocol: %hhu, nextProtocol: %hhu, nextUUID: %{public,uuid_t}.16P, nextStart: %hu, nextEnd: %hu, nextPktLen %hu, curSeqN: %hu, curStart: %hu, curEnd: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, NEXT_UUID, NEXT_RANGE_START, NEXT_RANGE_END, NEXT_LEN, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, LOG_SEQN)

  
                                                                        
   

   
                         
                                                                                        
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                            
                                                                                 
                                                                        
                                                                                                                  
   

#define MPKL_UUID_NEXT(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "9 {curProtocol: %hhu, nextProtocol: %hhu, curUUID: %{public,uuid_t}.16P, curPktLen: %u, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, CUR_LEN, LOG_SEQN)

   
                             
                                                                                        
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                            
                                                                                   
                                                                                 
                                                                                                                  
   

#define MPKL_SEQRANGE_NEXT(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, CUR_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "10 {curProtocol: %hhu, nextProtocol: %hhu, curUUID: %{public,uuid_t}.16P, curPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, CUR_LEN, LOG_SEQN)


   
                         
                                                                                            
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                              
                                                                                  
                                                                         
                                                                                                                  
   

#define MPKL_UUID_PREV(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_UUID, PREV_LEN, LOG_SEQN) \
os_log(LOGOBJECT, "11 {curProtocol: %hhu, prevProtocol: %hhu, prevUUID: %{public,uuid_t}.16P, prevPktLen: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, PREV_UUID, PREV_LEN, LOG_SEQN)

  
                                     
   

   
                          
                                          
  
                                                                     
                                                                                                                                                         
                                                                  
                                                                      
                                                                                            
                                                                                                                  
   

#define MPKL_TASK_START(LOGOBJECT, CLIENT_BUNDLE_ID, TASK_UUID, CONN_UUID, LOG_SEQN) \
os_log(LOGOBJECT, "12 {startBundleID: %@, taskUUID: %{public,uuid_t}.16P, connUUID: %{public,uuid_t}.16P, logSeqn: %hhu}", CLIENT_BUNDLE_ID, TASK_UUID, CONN_UUID, LOG_SEQN)

   
                          
                                        
  
                                                                     
                                                                                                                                                         
                                                                  
                                                                      
                                                                                            
                                                                                                                  
   

#define MPKL_TASK_END(LOGOBJECT, CLIENT_BUNDLE_ID, TASK_UUID, CONN_UUID, LOG_SEQN) \
os_log(LOGOBJECT, "13 {endBundleID: %@, taskUUID: %{public,uuid_t}.16P, connUUID: %{public,uuid_t}.16P, logSeqn: %hhu}", CLIENT_BUNDLE_ID, TASK_UUID, CONN_UUID, LOG_SEQN)

   
                                    
                                                                                                                                      
                                                                                                                                 
  
   

#define MPKL_SEQN_INCOMPLETE_PREV(LOGOBJECT, CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, PREV_RANGE_START, PREV_RANGE_END, LOG_SEQN) \
os_log(LOGOBJECT, "14 {Send Incomplete. curProtocol: %hhu, prevProtocol: %hhu, curSeqN: %hu, curStart: %hu, curEnd: %hu, prevStart: %hu, prevEnd: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, PREV_PROTOCOL_ID, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, PREV_RANGE_START, PREV_RANGE_END, LOG_SEQN)

   
                                    
                                                                                                                                      
                                                                                                                                 
  
   

#define MPKL_SEQN_INCOMPLETE_NEXT(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, NEXT_RANGE_START, NEXT_RANGE_END, LOG_SEQN) \
os_log(LOGOBJECT, "15 {Receive Incomplete. curProtocol: %hhu, nextProtocol: %hhu, curSeqN: %hu, curStart: %hu, curEnd: %hu, nextStart: %hu, nextEnd: %hu, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_SEQ_N, CUR_RANGE_START, CUR_RANGE_END, NEXT_RANGE_START, NEXT_RANGE_END, LOG_SEQN)

#ifdef KERNEL
   
                        
                                                                         
  
                                                                     
                                                                                                
                                                                                         
                                                                            
                                                                             
                                                                                                                       
                                                              
                                                                                       
                                                                                                                  
   
#define MPKL_TCP_SEND(LOGOBJECT, PREV_PROTOCOL_ID, PREV_UUID, LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_LEN, PID, LOG_SEQN)           \
	os_log_with_type(LOGOBJECT, net_mpklog_type,                                                                              \
	    "16 {curProtocol: 100, prevProtocol: %hhu, "                                                                          \
	    "prevUUID: "                                                                                                          \
	    "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X, "                                              \
	    "localPort: %hu, remotePort: %hu, tcpSeq: %u, length: %u, "                                                           \
	    "pid: %hu, logSeqn: %hhu}",                                                                                           \
	    PREV_PROTOCOL_ID,                                                                                                     \
	    PREV_UUID[0], PREV_UUID[1], PREV_UUID[2], PREV_UUID[3], PREV_UUID[4], PREV_UUID[5], PREV_UUID[6], PREV_UUID[7],       \
	    PREV_UUID[8], PREV_UUID[9], PREV_UUID[10], PREV_UUID[11], PREV_UUID[12], PREV_UUID[13], PREV_UUID[14], PREV_UUID[15], \
	    LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_LEN,                                                                            \
	    (uint16_t)PID, LOG_SEQN)

   
                         
                                                                                         
  
                                                                     
                                                                                       
                                                                                        
                                                                                            
                                                                                                   
                                                                                   
                                                                                  
                                                                                       
                                                                                                                  
   
#define MPKL_TCP_INPUT(LOGOBJECT, LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_ACK, TCP_LEN, TCP_FLAGS, PID, LOG_SEQN) \
	os_log_with_type(LOGOBJECT, net_mpklog_type,                                                            \
	    "17 {curProtocol: 100, prevProtocol: 80, "                                                          \
	    "localPort: %hu, remotePort: %hu, tcpSeq: %u, tcpAck: %u, tcpLen: %hu, tcpFlags: 0x%02x, "          \
	    "pid: %hu, logSeqn: %hhu}",                                                                         \
	    LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_ACK, (uint16_t)TCP_LEN, TCP_FLAGS,               \
	    (uint16_t)PID, LOG_SEQN)

   
                              
                                                                                    
  
                                                                     
                                                                       
                                                                                   
                                                                            
                                                                             
                                                                                            
                                                                                                   
                                                                                   
                                                                                  
   
#define MPKL_ESP_OUTPUT_TCP(LOGOBJECT, SPI, ESP_SEQ, LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_ACK, TCP_LEN, TCP_FLAGS)     \
	os_log_with_type(LOGOBJECT, (os_log_type_t)net_mpklog_type,                                                                    \
	    "18 {curProtocol: 80, spi: 0x%X, espSeq: %u, PayloadProtocol: 100, "                                        \
	    "localPort: %hu, remotePort: %hu, tcpSeq: %u, tcpAck: %u, tcpLen: %hu, tcpFlags: 0x%02x}",                  \
	    SPI, ESP_SEQ,                                                                                               \
	    LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_ACK, (uint16_t)TCP_LEN, TCP_FLAGS)

   
                             
                                                                     
  
                                                                     
                                                                       
                                                                                   
                                                                            
                                                                             
                                                                                            
                                                                                   
   
#define MPKL_ESP_INPUT_TCP(LOGOBJECT, SPI, ESP_SEQ, LOCAL_PORT, REMOTE_PORT, TCP_SEQ, TCP_LEN)  \
	os_log_with_type(LOGOBJECT, (os_log_type_t)net_mpklog_type,                                            \
	    "19 {curProtocol: 80 spi: 0x%X, espSeq: %u, PayloadProtocol: 100, "                 \
	    "localPort: %hu, remotePort: %hu, tcpSeq: %u, tcpLen: %hu}",                        \
	    SPI, ESP_SEQ,                                                                       \
	    LOCAL_PORT, REMOTE_PORT, TCP_SEQ, (uint16_t)TCP_LEN)
#endif             

   
                                        
                                                                                           
  
                                                                     
                                                                                                    
                                                                              
                                                                                          
                                                                          
                                                                        
                                                                                                                  
   
#define MPKL_BYTERANGE_UUID_ASSOCIATE(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, CUR_RANGE_START, CUR_RANGE_END, LOG_SEQN) \
os_log(LOGOBJECT, "32 {curProtocol: %hhu, nextProtocol: %hhu, curUUID: %{public}.16P, curStart: %llu, curEnd: %llu, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, CUR_RANGE_START, CUR_RANGE_END, LOG_SEQN)

   
                                        
                                                                      
  
                                                                     
                                                                                                                                                         
                                                                                                    
                                                                             
                                                                      
                                                                   
                                                                                                                  
   
#define MPKL_UUID_ONLY_ASSOCIATE_NEXT(LOGOBJECT, CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, NEXT_UUID, LOG_SEQN) \
os_log(LOGOBJECT, "33 {curProtocol: %hhu, nextProtocol: %hhu, curUUID: %{public}.16P, nextUUID: %{public}.16P, logSeqn: %hhu}", CUR_PROTOCOL_ID, NEXT_PROTOCOL_ID, CUR_UUID, NEXT_UUID, LOG_SEQN)

#ifdef KERNEL_PRIVATE
extern int net_mpklog_enabled;
extern uint8_t net_mpklog_type;
#endif                     

#endif                                  
