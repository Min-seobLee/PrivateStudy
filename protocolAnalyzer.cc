#include "protocolAnalyzer.h"
#include "HTML.h"
#include "timeChecker.h"
#include "math.h"

#define logB(x,base) log(x)/log(base)
int g_iTreadCount;
int g_iDiffSupp;
int g_iMinContentLength;
int g_iMaxFlowCount;
int g_iMaxMsgCount;
int g_iMaxSequenceLength;
int g_iMessageAssembleMethod;
int g_iMinSuppUnitMsg;
int g_iMinSuppUnitFlow;
int g_iSupp_Unit;
int g_iMinSuppUnitServer;
int g_irange;
int g_iStartOffset;
double g_iMaxSuppUnitPositionVariance;

int g_iDIREC_REQUESTMsgCount=0;
int g_iDIREC_RESPONSEMsgCount=0;
int g_iFlowCount=0;
int g_iFlowForwardCount=0;
int g_iFlowBackwardCount=0;


FPB	g_cTotalVolume;
FPB	g_cContentVolume;
FPB	g_cExclusiveContentVolume;
FPB	g_cPacketVolume;
FPB	g_cExclusivePacketVolume;
FPB	g_cFlowVolume;
FPB	g_cExclusiveFlowVolume;

u_int32_t g_uiContentID = 1;
u_int32_t g_uiMessageID = 1;
u_int32_t g_uiFlowID = 1;

//gyh-test!
//#################
int g_iRemnant_DIREC_REQUESTMsgCount=0;
int g_iRemnant_DIREC_RESPONSEMsgCount=0;
int g_iRemnant_FlowCount=0;
int g_iRemnant_FlowForwardCount=0;
int g_iRemnant_FlowBackwardCount=0;
int g_iTotalFwpFileCount=0;
//#################
void analyzeProtocol(char* p_caResultStructureDirectory, char* p_caResultTrafficDirectory, list<int> p_lConfigList, double iMaxSuppUnitPositionVariance,int p_iDebugFlag)
{
	puts("\nanalyzeProtocol() : start");

	TimeChecker				cTimeChecker;
	FlowHash				cFlowHash;

	int						iTotalFwpFileCount = 0;
	int						exitFlag=0;
	SequenceVector			cSiteSpecificSessionSequence;
	SequenceVector			cFlowSequence;
	SequenceVector			cMessageSequence;
	SequenceVector			cFieldSequence;

	char					caCMD[1024]={0,};
	char					caLogFileName[1024]={0,};
	char					caTraceInfoXmlFileName[1024]={0,};
	char					caMessageFormatXmlWithPacketFileName[1024]={0,};

	char					caResultTrafficFwpDirectory[1024]={0,};
	char					caResultDetailDirectory[1024]={0,};			//dir : /98-result/${argv1}_${argv2}/log

	char					caFieldSequenceDetail[1024]={0,};
	char					caMessageSequenceDetail[1024]={0,};
	char					caFlowSequenceDetail[1024]={0,};
	
	char					caFieldFormatDetail[1024]={0,};
	char					caMessageFormatDetail[1024]={0,};
	char					caFlowFormatDetail[1024]={0,};	

	char					caFieldFormatSnort[1024]={0,};
	char					caMessageFormatSnort[1024]={0,};
	char					caFlowFormatSnort[1024]={0,};	

	char					caFormatXmlFileName[1024]={0,};
	char					caFsmXmlFileName[1024]={0,};
	char					caDotFileName[1024]={0,};
	char					caPngFileName[1024]={0,};

	int						ret = 0;
	FieldFormatList			cDelimiterFieldFormatList;
	FieldFormatList			cFieldFormatList;
	FieldFormatList			cSplitedFieldFormatList;
	FieldFormatList			cRemnantFieldFormatList;
	MessageFormatList		cMessageFormatList;	
	FlowFormatList			cFlowFormatList;

	bool					bDelimiterExistent=false;
	bool					bSubDelimiterExistent=false;
	bool					bRemnantDelimiterExistent=false;
	bool					bTextProtocol=false;
	bool					bSomeRequestMessageSubDelimiterExistent=false;
	bool					bSomeResponseMessageSubDelimiterExistent=false;
	FSMManager				cFSMManager;
	HttpGTManager			cHttpGTManager;

	//create dir : /98-result/${argv1}_${argv2}/log
	sprintf(caResultDetailDirectory, "%s/log", p_caResultStructureDirectory);
	sprintf(caCMD, "rm -rf %s",caResultDetailDirectory);
	system(caCMD);

	ret = mkdir(caResultDetailDirectory, 0777);
	if (ret != 0) g_err((char*)"main() : protocol detail dir create error", 501);


	cTimeChecker.startClock();

	g_iTreadCount = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iDiffSupp = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMinContentLength = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMaxFlowCount = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMaxMsgCount = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMaxSequenceLength = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMessageAssembleMethod = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMinSuppUnitMsg = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMinSuppUnitFlow = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iSupp_Unit = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMinSuppUnitServer= p_lConfigList.front();	p_lConfigList.pop_front();
	g_irange= p_lConfigList.front();	p_lConfigList.pop_front();
	g_iStartOffset = p_lConfigList.front();	p_lConfigList.pop_front();
	g_iMaxSuppUnitPositionVariance=iMaxSuppUnitPositionVariance;


	//##################################################################
	//##################################################################
	//############### 트래픽 로드 & Message Assemble######################
	//##################################################################
	//##################################################################
	
	//traffic load		load traffic과 동시에 flow time순 정렬
	sprintf(caResultTrafficFwpDirectory, "%s/fwp",p_caResultTrafficDirectory);
	g_iTotalFwpFileCount = iTotalFwpFileCount = loadTraffic(&cFlowHash, caResultTrafficFwpDirectory, p_iDebugFlag);


	//Message Assemble 결과 출력
	sprintf(caLogFileName,"%s/message_assemble.txt",p_caResultStructureDirectory);
	cFlowHash.printMsgOfFlowHash(caLogFileName);

	//###################################################
	//###################################################
	//############### 시퀀스 추출 ######################
	//###################################################
	//###################################################








	//extract Site-SepcificSessionset Sequence
	extractSiteSpecificSessionsetSequence(&cSiteSpecificSessionSequence, &cFlowHash, p_iDebugFlag, &g_iFlowCount, &g_iFlowForwardCount, &g_iFlowBackwardCount);

	//extract flow sequence from fwp
	//**********flow는 최상위 단위이기 때문에 source에 아무것도 기입하지 않음, ex) (0:0:( 0 ))
	extractFlowSequence(&cSiteSpecificSessionSequence,&cFlowSequence, &cFlowHash, p_iDebugFlag, &g_iFlowCount, &g_iFlowForwardCount, &g_iFlowBackwardCount); 
//	cFlowSequence.print();
//	g_cTotalVolume.print();
	

	//extract message sequence from flow seuqnece
	//**********message sequence의 source(uiSourceID : flow sequence id, uiSourceIndex : flow에서 해당 message의 위치, uilSourceOffsetList : 0), ex) (1:1:( 0 )) flow id 1의 두번째 메시지
	extractMessageSequence(&cMessageSequence, &cFlowSequence, p_iDebugFlag, &g_iDIREC_REQUESTMsgCount, &g_iDIREC_RESPONSEMsgCount);
//	cMessageSequence.print();
	
	

	//extract keyword sequence from message sequence
	//**********keyword sequence의 source(uiSourceID : message sequence id, uiSourceIndex : 0,  uilSourceOffsetList : message내 시작 위치, 초기 Sequence에서는 모두 0), ex) (43:0:( 389 )) message id 43의 389번째 바이트에서 시작하는 콘텐츠 
	extractKeywordSequence(&cFieldSequence, &cMessageSequence, p_iDebugFlag);
//	cFieldSequence.print();

	//동일한 content를 가진 sequence들을 하나의 sequence로 구성, support, source 한쪽으로 합치고 ID 재설정, ->Header의 L4 Protocol만 Check하므로 Header만 다르고 Message의 payload가 같은 놈들을 하나로 합침
	uniqueKeywordSequence(&cFieldSequence, p_iDebugFlag);

	deleteDataMessageSequence(&cMessageSequence);
	//cFlowHash.printFlowList();
	if (p_iDebugFlag)
	{
		//make detail file name : /${argv1}_${argv2}/log/00-flow_sequence.txt
		sprintf(caFlowSequenceDetail, "%s/00-flow_sequence.txt", caResultDetailDirectory);
	//	puts(caFlowSequenceDetail);
		
		cFlowSequence.print(caFlowSequenceDetail);

		//make detail file name : /${argv1}_${argv2}/log/01-msg_sequence.txt
		sprintf(caMessageSequenceDetail, "%s/01-msg_sequence.txt", caResultDetailDirectory);
	//	puts(caMessageSequenceDetail);

		cMessageSequence.print(caMessageSequenceDetail);

		//make detail file name : /${argv1}_${argv2}/log/02-keyword_sequence.txt
		sprintf(caFieldSequenceDetail, "%s/02-keyword_sequence.txt",caResultDetailDirectory);
	//	puts(caContentSequenceDetail);

		cFieldSequence.print(caFieldSequenceDetail);
	}

		//================================================진행률 1/4====================================================
	setTextColorBlue();
	loadBar("\nAnalyzing Protocol          ", 1, 4, 4, 75);
	setTextColorRed();
	setTextTwinkle();
	printf("\nMessage Assemble          : Complete\n");
	setDefault();

	isTextProtocol(&cMessageSequence,&bTextProtocol);

	if(bTextProtocol)
	{
		extractDelimiterFieldFormat(&cFieldSequence, &cMessageSequence, &cDelimiterFieldFormatList, &bDelimiterExistent);
		//구분자가 존재한다면
		if(bDelimiterExistent)
		{
			splitFieldFormatbyDelimiter(&cFieldSequence, &cMessageSequence, &cSplitedFieldFormatList, &cDelimiterFieldFormatList);
			setTypeFieldFormatList(&cSplitedFieldFormatList);
			sprintf(caFormatXmlFileName,"%s/split_fieldformat_list.xml",p_caResultStructureDirectory);
			printFieldFormatListXML(caFormatXmlFileName, &cSplitedFieldFormatList);
			extractSubDelimiterFieldFormat(&cFieldFormatList,&cSplitedFieldFormatList,&cDelimiterFieldFormatList, &bSubDelimiterExistent);

			if(bSubDelimiterExistent)
			{
				splitFieldFormatbySubDelimiter(&cFieldFormatList,&cSplitedFieldFormatList, &cDelimiterFieldFormatList, &bSomeRequestMessageSubDelimiterExistent, &bSomeResponseMessageSubDelimiterExistent );	
				sprintf(caFormatXmlFileName,"%s/sub_split_fieldformat_list.xml",p_caResultStructureDirectory);
				printFieldFormatListXML(caFormatXmlFileName, &cFieldFormatList);
				extractRemnantDelimiterFieldFormat(&cFieldFormatList,&cDelimiterFieldFormatList, &bRemnantDelimiterExistent );
				
				if(bRemnantDelimiterExistent)
				{
					splitFieldFormatbyRemnantDelimiter(&cFieldFormatList,&cRemnantFieldFormatList,&cDelimiterFieldFormatList);

					sprintf(caFormatXmlFileName,"%s/remnant_fieldformat_list.xml",p_caResultStructureDirectory);
					printFieldFormatListXML(caFormatXmlFileName, &cRemnantFieldFormatList);
				}
				//Remnant Delimiter가 존재하지 않고 Request 메시지중에 SubDelimiter가 없는 메시지가 있거나 Response 메시지 중에 SubDelimiter가 없는 메시지가 있다면
				if(!bRemnantDelimiterExistent && (bSomeRequestMessageSubDelimiterExistent || bSomeResponseMessageSubDelimiterExistent))
				{
					//deDuplicationFieldFormat(&cFieldFormatList);
					extractRemnantFieldFormat(&cFieldFormatList);
				}
			
				
					
			}
			else
			{
				cFieldFormatList.insert(&cSplitedFieldFormatList);
			}

	
			
			deDuplicationFieldFormat(&cFieldFormatList);
			setTypeFieldFormatList(&cFieldFormatList);
			sprintf(caFormatXmlFileName,"%s/fieldformat_list.xml",p_caResultStructureDirectory);
			printFieldFormatListXML(caFormatXmlFileName, &cFieldFormatList);
			
			showInteractiveMenu(&cFlowHash, &cFlowSequence, &cMessageSequence, &cFieldSequence, &cFieldFormatList, &cMessageFormatList, &cFlowFormatList);			

			if (cMessageSequence.cvSequenceVector.front().clSequenceContentList.begin()->isHttpReqest() || cMessageSequence.cvSequenceVector.front().clSequenceContentList.begin()->isHttpResponse())
			{
				while(getchar()!='\n');
				cHttpGTManager.extractHttpField(&cMessageSequence);
				cHttpGTManager.verifyHTTP(&cFieldFormatList, &cMessageFormatList);

				ret = cHttpGTManager.generateHttpTrueMessageFormat(&cMessageSequence);

				cMessageFormatList.dCompression = cHttpGTManager.vcHttpTrueMessageFormat.size() > cMessageFormatList.clMessageFormat.size() ? 1 : (double)cHttpGTManager.vcHttpTrueMessageFormat.size()/cMessageFormatList.clMessageFormat.size();

				cHttpGTManager.printEvaluationSummary(&cFieldFormatList, &cMessageFormatList, &cMessageSequence);
			}

		}
		
	}

	//바이너리 프로토콜이거나 텍스트프로토콜이지만 구분자가 존재하지 않는 프로토콜이라면
	if((!bTextProtocol)||(bTextProtocol&&!bDelimiterExistent))
	{
		
				extractFieldFormat(&cSiteSpecificSessionSequence, &cFieldFormatList, &cFieldSequence, &cMessageSequence, iTotalFwpFileCount, p_iDebugFlag, g_iMinSuppUnitMsg, g_iDIREC_REQUESTMsgCount, g_iDIREC_RESPONSEMsgCount, g_iDIREC_REQUESTMsgCount+g_iDIREC_RESPONSEMsgCount, g_iMinSuppUnitFlow, g_iFlowCount, g_iSupp_Unit,g_iMinSuppUnitServer, g_iMaxSuppUnitPositionVariance); 


									//###################################################
									//###################################################
									//############ Field Format 임시 출력 ##################
									//###################################################
									//###################################################
									//Field Format 임시 출력
									//make detail file name : /04-result/field_format-temp.snort
									//sprintf(caFieldFormatSnort, "%s/field_format_befo_optimi-temp.snort", p_caResultDirectory);
									//	puts(caFieldFormatSnort);
									//cFieldFormatList.printSnort(caFieldFormatSnort);
									if (p_iDebugFlag)
									{
										//make detail file name : /${argv1}_${argv2}/log/03-field_format-temp.txt
										sprintf(caFieldFormatDetail, "%s/03-field_format_befo_optimi-temp.txt", caResultDetailDirectory);
									//	puts(caFieldFormatDetail);
									cFieldFormatList.clFieldFormat.sort(FieldFormat::CompareContent());
									cFieldFormatList.clFieldFormat.sort(FieldFormat::CompareContentDirec());
										//SuppMsg 내림차순 정렬
										cFieldFormatList.clFieldFormat.sort(FieldFormat::CompareSuppMsg());

										cFieldFormatList.print(caFieldFormatDetail);
									}
			//================================================진행률 2/4====================================================
			setTextColorBlue();
			loadBar("\nAnalyzing Protocol          ", 2, 4, 4, 75);
			setTextColorRed();
			setTextTwinkle();
			printf("\nSyntax Inference - Extract FieldFormat{SF(v)}          : Complete\n");
			setDefault();



			//###################################################
			//###################################################
			//############ Message Format 추출 ##################
			//###################################################
			//###################################################

					//Field Format의 source를 Field sequence의 source를 참조하여 message 기준으로 변경
					//Field sequence의 source는 여러개 일수 있기 때문에 각각에 대한 source 새로 생성하여 기입
					transferFormatSourceFormContentToMessage(&cFieldFormatList, &cFieldSequence, p_iDebugFlag);
					if (p_iDebugFlag)
					{
						//make detail file name : /04-result/log/04-field_format_transfer.txt
						sprintf(caFieldFormatDetail, "%s/04-field_format_transfer.txt", caResultDetailDirectory);
					
						cFieldFormatList.print(caFieldFormatDetail);
					}
					//deleteInclusionRelationFieldFormat(&cMessageSequence, &cFieldFormatList, g_iMinSuppUnitMsg, p_iDebugFlag);
					//단일 Message에 존재하는 Field Format ID로 구성된 sequence 생성
					extractFieldFormatIdSequence(&cMessageSequence, &cFieldFormatList, p_iDebugFlag);
					
					deleteInclusionRelationFieldFormat(&cMessageSequence, &cFieldFormatList, g_iMinSuppUnitFlow, p_iDebugFlag);
					if (p_iDebugFlag)
					{
						//make detail file name : /04-result/log/05-msg_sequence_sourceID.txt
						sprintf(caMessageSequenceDetail, "%s/05-msg_sequence_sourceID.txt", caResultDetailDirectory);
					

						cMessageSequence.print(caMessageSequenceDetail);
					}




		//gyh-test!-additional-1709200720
		//#################

				cFieldFormatList.setMinOffsetMaxDepthPositionVarFieldFormat();
				setTypeFieldFormatList(&cFieldFormatList);
				
				//FieldFormatXML출력
				

				if (p_iDebugFlag)
				{
					//make detail file name : /${argv1}_${argv2}/log/06-field_format-temp.txt
					sprintf(caFieldFormatDetail, "%s/06-field_format_after_optimi-temp.txt", caResultDetailDirectory);
				
					//SuppMsg 내림차순 정렬
					cFieldFormatList.clFieldFormat.sort(FieldFormat::CompareSuppMsg());

					cFieldFormatList.print(caFieldFormatDetail);
				}
									//Field Format 임시 출력
									//make detail file name : /04-result/field_format-temp.snort
									//sprintf(caFieldFormatSnort, "%s/field_format_after_optimi-temp.snort", p_caResultDirectory);
									//	puts(caFieldFormatSnort);
									//cFieldFormatList.printSnort(caFieldFormatSnort);
									cFieldFormatList.clFieldFormat.sort(FieldFormat::CompareContent());
									cFieldFormatList.clFieldFormat.sort(FieldFormat::CompareContentDirec());
		//#################
									cFieldFormatList.ArrangeID();

				sprintf(caFormatXmlFileName,"%s/fieldformat_list.xml",p_caResultStructureDirectory);
				printFieldFormatListXML(caFormatXmlFileName, &cFieldFormatList);


				if (g_irange==1)
				{
					cTimeChecker.endClock();
					puts("analyzeProtocol() - Field Format Inference: end");
					cTimeChecker.print();
					return;
				}



		//#################

			//추출
			extractMessageFormat(&cSiteSpecificSessionSequence,&cMessageFormatList, &cMessageSequence, &cFieldFormatList, iTotalFwpFileCount, p_iDebugFlag, g_iMinSuppUnitMsg, g_iDIREC_REQUESTMsgCount, g_iDIREC_RESPONSEMsgCount, g_iDIREC_REQUESTMsgCount+g_iDIREC_RESPONSEMsgCount, g_iMinSuppUnitFlow, g_iFlowCount, g_iSupp_Unit,g_iMinSuppUnitServer);
				
			//위치 설정		(Message Format의 FieldFormatList의 Offset 및 Depth Set, Depth는 Min Offset으로부터의 거리로
			setPositionMessageFormat(&cMessageFormatList, &cMessageSequence, p_iDebugFlag);
			
			//헤더 설정 , start도 같이 설정
			setHeaderMessageFormat(&cMessageFormatList, &cMessageSequence, p_iDebugFlag);
			
			//Message Format 내 Field Format 구분
			extractAdditionalFieldFormatInEachMessageFormat(&cMessageFormatList, &cMessageSequence, p_iDebugFlag);

					//임시 출력
					//make detail file name : /04-result/msg_format-temp.snort
					//sprintf(caMessageFormatSnort, "%s/msg_format-temp.snort", p_caResultDirectory);
					//	puts(caMessageFormatSnort);
					//cMessageFormatList.printSnort(caMessageFormatSnort);
					//cMessageFormatList.printSnortWithDynamicField(caMessageFormatSnort);
					if (p_iDebugFlag)
					{
						//make detail file name : /04-result/log/08-msg_format-temp.txt
						sprintf(caMessageFormatDetail, "%s/07-msg_format-temp.txt", caResultDetailDirectory);
					

						cMessageFormatList.print(caMessageFormatDetail);
					}


					cMessageFormatList.ArrangeID();

				//================================================진행률 3/4====================================================
			setTextColorBlue();
			loadBar("\nAnalyzing Protocol          ", 3, 4, 4, 75);
			setTextColorRed();
			setTextTwinkle();
			printf("\nSyntax Inference - Extract MessageFormat          : Complete\n");
			setDefault();

			//###################################################
			//###################################################
			//###################################################
			//############ 플로우 Format 추출 ################
			//###################################################
			//###################################################
			//###################################################

					//message Format의 source를 message sequence의 source를 참조하여 flow 기준으로 변경
					transferFormatSourceFormMessageToFlow(&cMessageFormatList, &cMessageSequence, p_iDebugFlag);
				
					if (p_iDebugFlag)
					{
						//make detail file name : /result/log/08-msg_format_transfer.txt
						sprintf(caMessageFormatDetail, "%s/08-msg_format_transfer.txt", caResultDetailDirectory);
					

						cMessageFormatList.print(caMessageFormatDetail);
					}

					//단일 플로우에 존재하는 Message Format ID로 구성된 sequence 생성
					extractMessageFormatIdSequence(&cFlowSequence, &cMessageFormatList, p_iDebugFlag);
				
					if (p_iDebugFlag)
					{
						//make detail file name : /result/log/09-flow_sequence_sourceID.txt
						sprintf(caFlowSequenceDetail, "%s/09-flow_sequence_sourceID.txt", caResultDetailDirectory);
					

						cFlowSequence.print(caFlowSequenceDetail);
					}

			//setCompletenessMessageFormat(&cFlowSequence, &cMessageSequence, &cMessageFormatList);
			setCoverageMessageFormat(&cFlowSequence, &cMessageSequence, &cMessageFormatList, &cFlowHash);
			sprintf(caFormatXmlFileName,"%s/messageformat_list.xml",p_caResultStructureDirectory);
			printMessageFormatListXML(caFormatXmlFileName, &cFieldFormatList, &cMessageFormatList);

			//messageformat_with_packet_list.xml 출력
			sprintf(caMessageFormatXmlWithPacketFileName,"%s/messageformat_with_packet_list.xml",p_caResultStructureDirectory);
			printMessageFormatListWithPacketXML(caMessageFormatXmlWithPacketFileName, &cMessageFormatList, &cFlowHash);

			//trace_info.xml 출력
			sprintf(caTraceInfoXmlFileName,"%s/trace_info.xml",p_caResultStructureDirectory);
			printTraceInfoXML(caTraceInfoXmlFileName, &cFlowHash, caResultTrafficFwpDirectory, &cMessageFormatList);

			if (g_irange==2)
			{
				cTimeChecker.endClock();
				puts("analyzeProtocol() - Message Format Inference: end");
				cTimeChecker.print();
				return;
			}	
			

			//추출
			extractFlowFormat(&cSiteSpecificSessionSequence,&cFlowFormatList, &cFlowSequence, &cMessageFormatList, iTotalFwpFileCount, p_iDebugFlag, g_iMinSuppUnitFlow, g_iFlowForwardCount, g_iFlowBackwardCount, g_iFlowCount, g_iSupp_Unit,g_iMinSuppUnitServer); 


			//위치 설정
			setPositionFlowFormat(&cFlowFormatList, &cMessageSequence, p_iDebugFlag);
			
			//헤더 설정 , start도 같이 설정
			setHeaderFlowFormat(&cFlowFormatList, &cMessageSequence, p_iDebugFlag);
			printf("flow Format count : %d\r\n",cFlowFormatList.clFlowFormat.size());

			

				//분석률
				setCompletenessFlowFormat(&cFlowFormatList, &cFlowSequence, &cFlowHash, p_iDebugFlag);

				//log
				printf("flow Format count : %d\r\n",cFlowFormatList.clFlowFormat.size());
				printf("flow Format completeness : %.2f(%llu/%llu) %.2f(%llu/%llu) %.2f(%llu/%llu)\r\n",
					(float)g_cFlowVolume.flow*100/g_cTotalVolume.flow, g_cFlowVolume.flow, g_cTotalVolume.flow,
					(float)g_cFlowVolume.pkt*100/g_cTotalVolume.pkt, g_cFlowVolume.pkt, g_cTotalVolume.pkt,
					(float)g_cFlowVolume.byte*100/g_cTotalVolume.byte, g_cFlowVolume.byte, g_cTotalVolume.byte);

			
				
				//임시 출력
				//make detail file name : /result/flow_format-temp.snort
				//sprintf(caFlowFormatSnort, "%s/flow_format-temp.snort", p_caResultDirectory);
				//	puts(caFlowFormatSnort);
				//cFlowFormatList.printSnort(caFlowFormatSnort);
				if (p_iDebugFlag)
				{
					//make detail file name : /result/log/10-flow_format-temp.txt
					sprintf(caFlowFormatDetail, "%s/10-flow_format-temp.txt", caResultDetailDirectory);
				

					cFlowFormatList.print(caFlowFormatDetail);
				}

				cFlowFormatList.ArrangeID();
				//================================================진행률 4/4====================================================
			setTextColorBlue();
			loadBar("\nAnalyzing Protocol          ", 4, 4, 4, 75);
			setTextColorRed();
			setTextTwinkle();
			printf("\nBehavior Inference - Extract Flow Format          : Complete\n");
			setDefault();

			sprintf(caFormatXmlFileName,"%s/flowformat_list.xml",p_caResultStructureDirectory);
			printFlowFormatListXML(caFormatXmlFileName, &cFieldFormatList, &cMessageFormatList, &cFlowFormatList);

			//format_list.xml 출력
			sprintf(caFormatXmlFileName,"%s/format_list.xml",p_caResultStructureDirectory);
			printFormatListXML(caFormatXmlFileName, &cFieldFormatList, &cMessageFormatList, &cFlowFormatList);

			cTimeChecker.endClock();
			puts("analyzeProtocol() : end");
			cTimeChecker.print();
			sprintf(caLogFileName,"%s/result_log.txt",p_caResultStructureDirectory);
			printResultLog(p_caResultTrafficDirectory, caLogFileName, &cFlowHash, &cFlowSequence, &cMessageSequence, &cFieldSequence, &cFieldFormatList, &cMessageFormatList, &cFlowFormatList);

			//setTextColorRed();
			//setTextTwinkle();
			//printf("Press return key to continue... \n");
			//setDefault();

			showInteractiveMenu(&cFlowHash, &cFlowSequence, &cMessageSequence, &cFieldSequence, &cFieldFormatList, &cMessageFormatList, &cFlowFormatList);			

		/*
			if (cMessageSequence.cvSequenceVector.front().clSequenceContentList.begin()->isHttpReqest() || cMessageSequence.cvSequenceVector.front().clSequenceContentList.begin()->isHttpResponse())
			{
				while(getchar()!='\n');
				cHttpGTManager.extractHttpField(&cMessageSequence);
				cHttpGTManager.verifyHTTP(&cFieldFormatList, &cMessageFormatList);

				ret = cHttpGTManager.generateHttpTrueMessageFormat(&cMessageSequence);

				cMessageFormatList.dCompression = cHttpGTManager.vcHttpTrueMessageFormat.size() > cMessageFormatList.clMessageFormat.size() ? 1 : (double)cHttpGTManager.vcHttpTrueMessageFormat.size()/cMessageFormatList.clMessageFormat.size();

				cHttpGTManager.printEvaluationSummary(&cFieldFormatList, &cMessageFormatList, &cMessageSequence);
			}
			*/
		

		

	}

	


	
		
	

	
	
	
	
	

	return;	
}
//########################################################################################
int loadTraffic(FlowHash* p_cpFlowHash, char* p_caTargetDirectory, int p_iDebugFlag)
{
	puts("loadTraffic() : start");

	TimeChecker			cTimeChecker;
	int					iTotalFwpFileCount = 0;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//############### 트래픽 로드 ######################
	//###################################################
	//###################################################
	//###################################################

	iTotalFwpFileCount = p_cpFlowHash->loadFlow(p_caTargetDirectory);			//p_caTargetDirectory에 저장되어 있는 모든 fwp 파일을 hash에 로드하고 파일 개수 리턴 
																				//flowHash.cc -> 1289번째줄
	p_cpFlowHash->setMessageContainerDoublyListofFlowHash(g_iMessageAssembleMethod);	//FlowHash에 Message 구성
	//p_cpFlowHash->printMsgOfFlowHashToDebug();
	printf("iTotalFwpFileCount : %d\n", iTotalFwpFileCount);

	p_cpFlowHash->resetFlowListSortByTime();
	p_cpFlowHash->resetMsgListSortById();
	//p_cpFlowHash->printFlowList();
	cTimeChecker.endClock();
	cTimeChecker.print();

	return iTotalFwpFileCount;
}

//########################################################################################
int extractSiteSpecificSessionsetSequence(SequenceVector* p_cpSiteSpecificSessionsetSequenceVector, FlowHash* p_cpFlowHash, int p_iDebugFlag, int *p_iFlowCount, int *p_iFlowForwardCount, int *p_iFlowBackwardCount)
{
	puts("extractSiteSpecificSessionsetSequence() : start");

	TimeChecker			cTimeChecker;
	vector<u_int32_t>::iterator		itTemp;
	cTimeChecker.startClock();
	vector<Sequence>::iterator		itSeq;

	p_cpSiteSpecificSessionsetSequenceVector->extractSiteSpecificSessionsetSequenceForFlowSequence(p_cpFlowHash, &g_cTotalVolume, g_iMaxFlowCount, g_iMaxMsgCount, g_iMaxSequenceLength, p_iFlowCount, p_iFlowForwardCount, p_iFlowBackwardCount);
	
	
	cTimeChecker.endClock();

	puts("extractSiteSpecificSessionsetSequence() : end");
	cTimeChecker.print();

	return p_cpSiteSpecificSessionsetSequenceVector->cvSequenceVector.size();
}
//########################################################################################
int extractFlowSequence(SequenceVector* p_cpSiteSpecificSessionsetSequenceVector,SequenceVector* p_cpFlowSequenceVector, FlowHash* p_cpFlowHash, int p_iDebugFlag, int *p_iFlowCount, int *p_iFlowForwardCount, int *p_iFlowBackwardCount)
{
	puts("extractFlowSequence() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpFlowSequenceVector->extractFlowSequenceForMessageSequence(p_cpSiteSpecificSessionsetSequenceVector,p_cpFlowHash, &g_cTotalVolume, g_iMaxFlowCount, g_iMaxMsgCount, g_iMaxSequenceLength, p_iFlowCount, p_iFlowForwardCount, p_iFlowBackwardCount,g_iStartOffset);
	//extractFlowSequenceForMessageSequence -> sequence.cc 2581번째줄
	
	cTimeChecker.endClock();

	puts("extractFlowSequence() : end");
	cTimeChecker.print();

	return p_cpFlowSequenceVector->cvSequenceVector.size();
}
//########################################################################################
int extractMessageSequence(SequenceVector* p_cpMsgSequenceVector, SequenceVector* p_cpFlowSequenceVector, int p_iDebugFlag, int * p_iDIREC_REQUESTMsgCount, int *p_iDIREC_RESPONSEMsgCount)
{
	puts("extractMessageSequence() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpMsgSequenceVector->extractMessageSequence(p_cpFlowSequenceVector, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);
	//extractMessageSequence -> sequence.cc 2703번째줄
	printf("Total Message Sequence Count : %d\n", p_cpMsgSequenceVector->cvSequenceVector.size());

	cTimeChecker.endClock();

	puts("extractMessageSequence() : end");
	cTimeChecker.print();

	return p_cpMsgSequenceVector->cvSequenceVector.size();
}

//########################################################################################
int extractKeywordSequence(SequenceVector* p_cpFieldSequenceVector, SequenceVector* p_cpMessageSequenceVector, int p_iDebugFlag)
{
	puts("extractKeywordSequence() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpFieldSequenceVector->extractContentSequence(p_cpMessageSequenceVector);
	
	printf("Total Field Sequence Count : %d\n", p_cpFieldSequenceVector->cvSequenceVector.size());

	cTimeChecker.endClock();
	
	puts("extractKeywordSequence() : end");
	cTimeChecker.print();

	return p_cpFieldSequenceVector->cvSequenceVector.size();
}
//########################################################################################
int uniqueKeywordSequence(SequenceVector* p_cpFieldSequenceVector, int p_iDebugFlag)
{
	puts("uniqueKeywordSequence() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpFieldSequenceVector->uniqueContentSequence();
	
	printf("Total Keyword Sequence Count : %d\n", p_cpFieldSequenceVector->cvSequenceVector.size());

	cTimeChecker.endClock();

	puts("uniqueKeywordSequence() : end");
	cTimeChecker.print();

	return p_cpFieldSequenceVector->cvSequenceVector.size();
}
//########################################################################################
int extractFieldFormat(SequenceVector* p_cpSiteSpecificSessionsetSequenceVector, FieldFormatList* p_cpFieldFormatList,  SequenceVector* p_cpContentSequenceVector, SequenceVector* p_cpMessageSequenceVector, int p_iTotalFwpFileCount, int p_iDebugFlag, int p_iMinSuppUnitMsg, int p_iDIREC_REQUESTMsgCount, int p_iDIREC_RESPONSEMsgCount, int p_iTotalMsgCount, int p_iMinSuppUnitFlow, int p_iFlowCount, int p_iSupp_Unit, int p_iMinSuppUnitServer,double g_iMaxSuppUnitPositionVariance)
{
	puts("extractFieldFormat() : start");

	TimeChecker				cTimeChecker;
	int						iTargetSupp = 0;
	FieldFormatList			cFieldFormatListLengthK;
	FieldFormatList			cFieldFormatListLengthK_1;
	int						iContentLength = 0;
	int						SessionSetCount=0;
	cTimeChecker.startClock();

	//default minimum file support is 2
	iTargetSupp = p_iTotalFwpFileCount - g_iDiffSupp;
	if (iTargetSupp < 2)
		iTargetSupp = 2;

	//###################################################
	//###################################################
	//###################################################
	//################## 필드 포맷 추출 ###################
	//###################################################
	//###################################################
	//###################################################

	switch(p_iSupp_Unit)	
	{	// 0 : message_total, 1 : message_direction, 2 : Flow(포함관계 제거 알고리즘 X), 3 : FILE
		case 0:		//Supp_unit = message_total
					//길이 1인 후보 Format 추출
					p_cpContentSequenceVector->extractFieldFormatLength1_Message_Total(&cFieldFormatListLengthK_1, &g_uiContentID);
					
					//용의자 리스트 구성	//gyh-check - 1짜리에서 source에 ID, offsetlist 구축
					p_cpContentSequenceVector->constructContentSuspectlist_Message_Total(&cFieldFormatListLengthK_1);

					//분석하는 message id list 구성 -> size() = message 단위 support		//모든 Format들 순회 -> Format의 소스들 순회 -> Format 소스의 ID를 가진 필드 시퀀스 순회 -> 필드 시퀀스의 SourceID가 MessageID
					p_cpContentSequenceVector->constructIdentifiedMessageIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//분석하는 message id and Flow id list 구성 -> size() = flow 단위 support
					//p_cpContentSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1, p_cpMessageSequenceVector);

					//파일 지지도 계산
					p_cpContentSequenceVector->yieldContentSupport_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//파일 지지도 미만 Format 삭제
					//p_cpContentSequenceVector->deleteUnderSupport(&cFieldFormatListLengthK_1, iTargetSupp);

					//message_total 단위의 지지도 미만 Format 삭제
					p_cpContentSequenceVector->deleteUnderSupport_unit_Message_Total(&cFieldFormatListLengthK_1, p_iMinSuppUnitMsg, p_iTotalMsgCount);

					//길이 K 후보 Format 추출
					iContentLength=2;
					
					do
					{
						printf("Format num k-1: %d\n",cFieldFormatListLengthK_1.clFieldFormat.size());
						printf("Format num final: %d\n",p_cpFieldFormatList->clFieldFormat.size());

						//K-1를 사용하여 K 생성
						p_cpContentSequenceVector->extractFieldFormatLengthK_hash_Message_Total(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitMsg, p_iTotalMsgCount);

						//불필요한 Format 삭제
						cFieldFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpFieldFormatList->insert(&cFieldFormatListLengthK_1);

						//g_iMinContentLength 보다 짮은 콘텐츠 Format 삭제
						if (g_iMinContentLength == iContentLength)
						{
							trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);
						}

						//K-1 비움
						cFieldFormatListLengthK_1.reset();

						//K  K-1로 복사
						cFieldFormatListLengthK_1.insert(&cFieldFormatListLengthK);

						//K 비움
						cFieldFormatListLengthK.reset();
						
						iContentLength++;
					}
					while (cFieldFormatListLengthK_1.clFieldFormat.size());
					



			break;
		case 1:		//Supp_unit = message_direction
					//길이 1인 후보 Format 추출
					p_cpContentSequenceVector->extractFieldFormatLength1_Message_Direction(&cFieldFormatListLengthK_1, &g_uiContentID);
					
					//용의자 리스트 구성	//gyh-check - 1짜리에서 source에 ID, offsetlist 구축
					p_cpContentSequenceVector->constructContentSuspectlist_Message_Direction(&cFieldFormatListLengthK_1);

					//분석하는 message id list 구성 -> size() = message 단위 support		//모든 Format들 순회 -> Format의 소스들 순회 -> Format 소스의 ID를 가진 키워드 시퀀스 순회 -> 키워드 시퀀스의 SourceID가 MessageID
					p_cpContentSequenceVector->constructIdentifiedMessageIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//분석하는 message id and Flow id list 구성 -> size() = flow 단위 support
					//p_cpContentSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1, p_cpMessageSequenceVector);

					//파일 지지도 계산
					p_cpContentSequenceVector->yieldContentSupport_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//파일 지지도 미만 Format 삭제
					//p_cpContentSequenceVector->deleteUnderSupport(&cFieldFormatListLengthK_1, iTargetSupp);

					//message_direction 단위의 지지도 미만 Format 삭제        sequence.cc 2728번째줄
					p_cpContentSequenceVector->deleteUnderSupport_unit_Message_Direction(&cFieldFormatListLengthK_1, p_iMinSuppUnitMsg, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);
					

					//길이 K 후보 Format 추출
					iContentLength=2;
					
					do
					{
						printf("Format num k-1: %d\n",cFieldFormatListLengthK_1.clFieldFormat.size());
						printf("Format num final: %d\n",p_cpFieldFormatList->clFieldFormat.size());

						//K-1를 사용하여 K 생성
						//p_cpContentSequenceVector->extractFieldFormatLengthK_hash(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID);
						//FLlist까지 생성할때 - gyh-check-warning!

						
						//p_cpContentSequenceVector->extractFieldFormatLengthK_hash_Message_Total(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitMsg, p_iTotalMsgCount);
						p_cpContentSequenceVector->extractFieldFormatLengthK_hash_Message_Directoin(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitMsg, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);

						//불필요한 Format 삭제
						cFieldFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpFieldFormatList->insert(&cFieldFormatListLengthK_1);

						//g_iMinContentLength 보다 짮은 콘텐츠 Format 삭제
						if (g_iMinContentLength == iContentLength)
						{
							trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);
						}

						//K-1 비움
						cFieldFormatListLengthK_1.reset();

						//K  K-1로 복사
						cFieldFormatListLengthK_1.insert(&cFieldFormatListLengthK);

						//K 비움
						cFieldFormatListLengthK.reset();
						
						iContentLength++;
					}
					while (cFieldFormatListLengthK_1.clFieldFormat.size());
			break;
		case 2:		//Supp_unit = flow
					//길이 1인 후보 Format 추출
					p_cpContentSequenceVector->extractFieldFormatLength1_Message_Total(&cFieldFormatListLengthK_1, &g_uiContentID);
					
					//용의자 리스트 구성	//gyh-check - 1짜리에서 source에 ID, offsetlist 구축
					p_cpContentSequenceVector->constructContentSuspectlist_Message_Total(&cFieldFormatListLengthK_1);

					//분석하는 message id list 구성 -> size() = message 단위 support		//모든 Format들 순회 -> Format의 소스들 순회 -> Format 소스의 ID를 가진 필드 시퀀스 순회 -> 필드 시퀀스의 SourceID가 MessageID
					//p_cpContentSequenceVector->constructIdentifiedMessageIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//분석하는 message id and Flow id list 구성 -> size() = flow 단위 support
					p_cpContentSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1, p_cpMessageSequenceVector);

					//파일 지지도 계산
					p_cpContentSequenceVector->yieldContentSupport_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//파일 지지도 미만 Format 삭제
					//p_cpContentSequenceVector->deleteUnderSupport(&cFieldFormatListLengthK_1, iTargetSupp);

					//message 단위의 지지도 미만 Format 삭제
					//p_cpContentSequenceVector->deleteUnderSupport_unit_Message_Total(&cFieldFormatListLengthK_1, p_iMinSuppUnitMsg, p_iTotalMsgCount);

					//flow 단위의 지지도 미만 Format 삭제
					p_cpContentSequenceVector->deleteUnderSupport_unit_Flow(&cFieldFormatListLengthK_1, p_iMinSuppUnitFlow, p_iFlowCount);

					//길이 K 후보 Format 추출
					iContentLength=2;
					
					do
					{
						printf("Format num k-1: %d\n",cFieldFormatListLengthK_1.clFieldFormat.size());
						printf("Format num final: %d\n",p_cpFieldFormatList->clFieldFormat.size());

						//K-1를 사용하여 K 생성
						p_cpContentSequenceVector->extractFieldFormatLengthK_hash_Message_Total_prunning_Flow(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitFlow, p_iFlowCount);

						//불필요한 Format 삭제 gyh-check-warning!//flow 단위의 포함관계 제거가 아니고 message 단위의 포함관계 제거
						cFieldFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpFieldFormatList->insert(&cFieldFormatListLengthK_1);

						//g_iMinContentLength 보다 짮은 콘텐츠 Format 삭제
						if (g_iMinContentLength == iContentLength)
						{
							trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);
						}

						//K-1 비움
						cFieldFormatListLengthK_1.reset();

						//K  K-1로 복사
						cFieldFormatListLengthK_1.insert(&cFieldFormatListLengthK);

						//K 비움
						cFieldFormatListLengthK.reset();
						
						iContentLength++;
					}
					while (cFieldFormatListLengthK_1.clFieldFormat.size());
			break;
		case 3:		//Supp_unit = File
					//길이 1인 후보 Format 추출
					p_cpContentSequenceVector->extractFieldFormatLength1_Message_Total(&cFieldFormatListLengthK_1, &g_uiContentID);
					
					//용의자 리스트 구성	//gyh-check - 1짜리에서 source에 ID, offsetlist 구축
					p_cpContentSequenceVector->constructContentSuspectlist_Message_Total(&cFieldFormatListLengthK_1);

					//분석하는 message id list 구성 -> size() = message 단위 support		//모든 Format들 순회 -> Format의 소스들 순회 -> Format 소스의 ID를 가진 필드 시퀀스 순회 -> 필드 시퀀스의 SourceID가 MessageID
					p_cpContentSequenceVector->constructIdentifiedMessageIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//분석하는 message id and Flow id list 구성 -> size() = flow 단위 support
					//p_cpContentSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1, p_cpMessageSequenceVector);

					//파일 지지도 계산
					p_cpContentSequenceVector->yieldContentSupport_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//파일 지지도 미만 Format 삭제
					p_cpContentSequenceVector->deleteUnderSupport(&cFieldFormatListLengthK_1, iTargetSupp);

					//message 단위의 지지도 미만 Format 삭제
					//p_cpContentSequenceVector->deleteUnderSupport_unit_Message_Total(&cFieldFormatListLengthK_1, p_iMinSuppUnitMsg, p_iTotalMsgCount);

					//길이 K 후보 Format 추출
					iContentLength=2;
					
					do
					{
						printf("Format num k-1: %d\n",cFieldFormatListLengthK_1.clFieldFormat.size());
						printf("Format num final: %d\n",p_cpFieldFormatList->clFieldFormat.size());

						//K-1를 사용하여 K 생성
						p_cpContentSequenceVector->extractFieldFormatLengthK_hash_Message_Total_prunning_File(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitMsg, p_iTotalMsgCount);

						//불필요한 Format 삭제
						cFieldFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpFieldFormatList->insert(&cFieldFormatListLengthK_1);

						//g_iMinContentLength 보다 짮은 콘텐츠 Format 삭제
						if (g_iMinContentLength == iContentLength)
						{
							trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);
						}

						//K-1 비움
						cFieldFormatListLengthK_1.reset();

						//K  K-1로 복사
						cFieldFormatListLengthK_1.insert(&cFieldFormatListLengthK);

						//K 비움
						cFieldFormatListLengthK.reset();
						
						iContentLength++;
					}
					while (cFieldFormatListLengthK_1.clFieldFormat.size());
			break;
		case 4 :
					//Supp_unit = 세션 서포트, 특정사이트 세션 집합 서포트
					//길이 1인 후보 Format 추출
					//p_cpContentSequenceVector 는 필드시퀀스
					p_cpContentSequenceVector->extractFieldFormatLength1_Message_Direction(&cFieldFormatListLengthK_1, &g_uiContentID);
					
					//용의자 리스트 구성	//gyh-check - 1짜리에서 source에 ID, offsetlist 구축
					p_cpContentSequenceVector->constructContentSuspectlist_Message_Direction(&cFieldFormatListLengthK_1);

					//분석하는 message id and Flow id list 구성 -> size() = flow 단위 support
					p_cpContentSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1, p_cpMessageSequenceVector);

					//파일 지지도 계산
					p_cpContentSequenceVector->yieldContentSupport_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//세션단위,site-specific session set 지지도 미만 Format 삭제(수정해야할 부분)
					SessionSetCount=p_cpContentSequenceVector->deleteUnderSupport_unit_AutoReEngine(p_cpSiteSpecificSessionsetSequenceVector,&cFieldFormatListLengthK_1,p_cpMessageSequenceVector, p_iMinSuppUnitFlow, p_iFlowCount,p_iMinSuppUnitServer,p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount,g_iMinSuppUnitMsg);

					//길이 K 후보 Format 추출
					iContentLength=2;
					
					do
					{
						printf("Format num k-1: %d\n",cFieldFormatListLengthK_1.clFieldFormat.size());
						printf("Format num final: %d\n",p_cpFieldFormatList->clFieldFormat.size());

						//K-1를 사용하여 K 생성(수정해야할 부분)
						p_cpContentSequenceVector->extractFieldFormatLengthK_prunning_AutoReEngine(p_cpSiteSpecificSessionsetSequenceVector,&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitFlow, p_iFlowCount,p_iMinSuppUnitServer,p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount,g_iMinSuppUnitMsg);

						//불필요한 Format 삭제 gyh-check-warning!//flow 단위의 포함관계 제거가 아니고 message 단위의 포함관계 제거
						cFieldFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpFieldFormatList->insert(&cFieldFormatListLengthK_1);
						
						
						//g_iMinContentLength(4) 보다 짮은 콘텐츠 Format 삭제
						if (g_iMinContentLength == iContentLength)
						{
							trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);
						}
						

						//K-1 비움
						cFieldFormatListLengthK_1.reset();

						//K  K-1로 복사
						cFieldFormatListLengthK_1.insert(&cFieldFormatListLengthK);

						//K 비움
						cFieldFormatListLengthK.reset();
						
						iContentLength++;
					}
					while (cFieldFormatListLengthK_1.clFieldFormat.size());
					printf("FieldFormat Size = %d",p_cpFieldFormatList->clFieldFormat.size());
					//getchar();
					p_cpMessageSequenceVector->VarianceAnalysis(p_cpFieldFormatList,g_iMaxSuppUnitPositionVariance);



					
			break;
		default:	//Supp_unit = message
					//길이 1인 후보 Format 추출
					p_cpContentSequenceVector->extractFieldFormatLength1_Message_Total(&cFieldFormatListLengthK_1, &g_uiContentID);
					
					//용의자 리스트 구성	//gyh-check - 1짜리에서 source에 ID, offsetlist 구축
					p_cpContentSequenceVector->constructContentSuspectlist_Message_Total(&cFieldFormatListLengthK_1);

					//분석하는 message id list 구성 -> size() = message 단위 support		//모든 Format들 순회 -> Format의 소스들 순회 -> Format 소스의 ID를 가진 필드 시퀀스 순회 -> 필드 시퀀스의 SourceID가 MessageID
					p_cpContentSequenceVector->constructIdentifiedMessageIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//분석하는 message id and Flow id list 구성 -> size() = flow 단위 support
					//p_cpContentSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_FieldFormatList(&cFieldFormatListLengthK_1, p_cpMessageSequenceVector);

					//파일 지지도 계산
					p_cpContentSequenceVector->yieldContentSupport_For_FieldFormatList(&cFieldFormatListLengthK_1);

					//파일 지지도 미만 Format 삭제
					//p_cpContentSequenceVector->deleteUnderSupport(&cFieldFormatListLengthK_1, iTargetSupp);

					//message 단위의 지지도 미만 Format 삭제
					p_cpContentSequenceVector->deleteUnderSupport_unit_Message_Total(&cFieldFormatListLengthK_1, p_iMinSuppUnitMsg, p_iTotalMsgCount);

					//길이 K 후보 Format 추출
					iContentLength=2;
					
					do
					{
						printf("Format num k-1: %d\n",cFieldFormatListLengthK_1.clFieldFormat.size());
						printf("Format num final: %d\n",p_cpFieldFormatList->clFieldFormat.size());

						//K-1를 사용하여 K 생성
						p_cpContentSequenceVector->extractFieldFormatLengthK_hash_Message_Total(&cFieldFormatListLengthK, &cFieldFormatListLengthK_1, iContentLength, iTargetSupp, &g_uiContentID, p_cpMessageSequenceVector, p_iMinSuppUnitMsg, p_iTotalMsgCount);

						//불필요한 Format 삭제
						cFieldFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpFieldFormatList->insert(&cFieldFormatListLengthK_1);

						//g_iMinContentLength(4) 보다 짮은 콘텐츠 Format 삭제
						if (g_iMinContentLength == iContentLength)
						{
							trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);
						}

						//K-1 비움
						cFieldFormatListLengthK_1.reset();

						//K  K-1로 복사
						cFieldFormatListLengthK_1.insert(&cFieldFormatListLengthK);

						//K 비움
						cFieldFormatListLengthK.reset();
						
						iContentLength++;
					}
					while (cFieldFormatListLengthK_1.clFieldFormat.size());
			break;
	}

	trimUnderContentLength(p_cpFieldFormatList, p_iDebugFlag);

	printf("FieldFormatCount : %d\n",p_cpFieldFormatList->clFieldFormat.size());

	cTimeChecker.endClock();

	puts("extractFieldFormat() : end");
	cTimeChecker.print();

	return p_cpFieldFormatList->clFieldFormat.size();
}
//########################################################################################
int trimUnderContentLength(FieldFormatList* p_cpFieldFormatList, int p_iDebugFlag)
{
	puts("trimUnderContentLength() : start");

	TimeChecker				cTimeChecker;
	int						iTargetSupp = 0;
	FieldFormatList	cFieldFormatListLengthK;
	FieldFormatList	cFieldFormatListLengthK_1;
	int						iContentLength = 0;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//##### 기준 길이 미달 콘텐츠 Format 삭제 #######
	//###################################################
	//###################################################
	//###################################################

	
	p_cpFieldFormatList->trimUnderContentLength(g_iMinContentLength);
	printf("FieldFormatCount : %d\n",p_cpFieldFormatList->clFieldFormat.size());

	cTimeChecker.endClock();

	puts("trimUnderContentLength() : end");
	cTimeChecker.print();

	return p_cpFieldFormatList->clFieldFormat.size();
}
//########################################################################################
void transferFormatSourceFormContentToMessage(FieldFormatList* p_cpFieldFormatList,  SequenceVector* p_cpContentSequenceVector, int p_iDebugFlag)
{
	puts("transferFormatSourceFormContentToMessage() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//############ Format 소스 변경 ##################
	//###################################################
	//###################################################
	//###################################################

	p_cpContentSequenceVector->transferSource(p_cpFieldFormatList);

	cTimeChecker.endClock();

	puts("transferFormatSourceFormContentToMessage() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void extractFieldFormatIdSequence(SequenceVector* p_cpSequence, FieldFormatList* p_cpFieldFormatList, int p_iDebugFlag)
{
	puts("extractFieldFormatIdSequence() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();


	//###################################################
	//###################################################
	//###################################################
	//########## Format ID 시퀀스 생성 ###############
	//###################################################
	//###################################################
	//###################################################

	p_cpSequence->extractFieldFormatIdSequence(p_cpFieldFormatList);

	cTimeChecker.endClock();

	puts("extractFieldFormatIdSequence() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void deleteInclusionRelationFieldFormat(SequenceVector* p_cpMsgSequenceVector, FieldFormatList* p_cpFieldFormatList, int p_iMinSuppUnitMsg, int p_iDebugFlag)
{
	puts("deleteInclusionRelationFieldFormat() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();


	//###################################################
	//###################################################
	//###################################################
	//########## 앞 모듈에서 제거 안 된 포함관계 제거 #######
	//###################################################
	//###################################################
	//###################################################

	p_cpFieldFormatList->deleteInclusionRelationFieldFormat(p_iMinSuppUnitMsg);

	p_cpMsgSequenceVector->arrangeFormatIDListFinal(p_cpFieldFormatList);

	printf("FieldFormatCount : %d\n",p_cpFieldFormatList->clFieldFormat.size());

	cTimeChecker.endClock();

	puts("deleteInclusionRelationFieldFormat() : end");
	cTimeChecker.print();
	
	return;

}
//########################################################################################
void setTypeFieldFormatList(FieldFormatList* p_cpFieldFormatList)
{
	puts("setTypeFieldFormatList() : start");

	TimeChecker				cTimeChecker;
	list<FieldFormat>::iterator				itFieldFormat;

	cTimeChecker.startClock();

	for(itFieldFormat =	p_cpFieldFormatList->clFieldFormat.begin() ; itFieldFormat != p_cpFieldFormatList->clFieldFormat.end() ; ++itFieldFormat)
	{
		
		//if(itFieldFormat->cvAdditionalContentVector.size()) //새로운 값들이 추가되었다면 이 SF(v)는 여러개의 값을 가지는 필드포맷이므로 DF(v)
	//	{
		//	itFieldFormat->uiType |= DFV_TYPE;   //or연산 후 할당
		//	printf("FieldFormat AdditionalContentVector size : %d\n",itFieldFormat->cvAdditionalContentVector.size());
		//}
			
		//else
		//{	
			itFieldFormat->uiType |= SFV_TYPE;
			
		//}
	//	printf("Field Format uiOffset : %d\n",itFieldFormat->cContent.uiOffset);
			
	}

	//error check
	for(itFieldFormat =	p_cpFieldFormatList->clFieldFormat.begin() ; itFieldFormat != p_cpFieldFormatList->clFieldFormat.end() ; ++itFieldFormat)
	{
		//itFieldFormat->cContent.printContent();
		//printf("itFieldFormat->uiType = %#x",itFieldFormat->uiType);
		//getchar();
		if(!itFieldFormat->uiType & SFV_TYPE)
		{
			printf("itFieldFormat->uiType = %#x",itFieldFormat->uiType);
			g_err((char*)"setTypeFieldFormatList() : type setting error");
		}
	}

	cTimeChecker.endClock();

	puts("setTypeFieldFormatList() : end");
	cTimeChecker.print();

	return;
}
//########################################################################################
int extractMessageFormat(SequenceVector* p_cpSiteSpecificSessionsetSequenceVector,MessageFormatList* p_cpMessageFormatList,  SequenceVector* p_cpMessageSequenceVector, FieldFormatList* p_cpFieldFormatList, int p_iTotalFwpFileCount, int p_iDebugFlag, int p_iMinSuppUnitMsg, int p_iDIREC_REQUESTMsgCount, int p_iDIREC_RESPONSEMsgCount, int p_iTotalMsgCount, int p_iMinSuppUnitFlow, int p_iFlowCount, int p_iSupp_Unit, int p_iMinSuppUnitServer)
{
	puts("extractMessageFormat() : start");

	TimeChecker				cTimeChecker;
	int						iTargetSupp = 0;
	MessageFormatList		cMessageFormatListLengthK;
	MessageFormatList		cMessageFormatListLengthK_1;
	int						iMessageLength = 0;
	int						Site_count=0;
	cTimeChecker.startClock();

	//minimum file support is 2
	iTargetSupp = p_iTotalFwpFileCount - g_iDiffSupp;
	if (iTargetSupp < 2)
		iTargetSupp = 2;

	//###################################################
	//###################################################
	//###################################################
	//############## Message Format 추출 ################
	//###################################################
	//###################################################
	//###################################################

	switch(p_iSupp_Unit)	
	{	// 0 : message_total, 1 : message_direction, 2 : Flow, 3 : FILE	-> 아직 적용 안함
		case 0:		//Supp_unit = message_total
					//길이 1인 후보 Format 추출	(Field Format 1개짜리로 이루어진 Message Format 구성)
					cMessageFormatListLengthK_1.insert(p_cpFieldFormatList, &g_uiMessageID);
							
					//용의자 리스트 구성
					p_cpMessageSequenceVector->constructMessageSuspectlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//분석하는 message id list 구성
					p_cpMessageSequenceVector->constructIdentifiedMessageIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);
					//분석하는 message id list와 flow id list 구성
					//p_cpMessageSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//파일 지지도 계산
					p_cpMessageSequenceVector->yieldMessageSupport_For_MessageFormatList(&cMessageFormatListLengthK_1);
					
					//파일 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport(&cMessageFormatListLengthK_1, iTargetSupp);

					//message_total 단위의 지지도 미만 Format 삭제
					p_cpMessageSequenceVector->deleteUnderSupport_unit_Message_Total(&cMessageFormatListLengthK_1, p_iMinSuppUnitMsg, p_iTotalMsgCount);
				
					//길이 K 후보 Format 추출
					iMessageLength=2;
					
					do
					{
						printf("Format num K-1: %d\n",cMessageFormatListLengthK_1.clMessageFormat.size());
						printf("Format num final: %d\n",p_cpMessageFormatList->clMessageFormat.size());

						//K-1를 사용하여 K 생성
						p_cpMessageSequenceVector->extractMessageFormatLengthK_Message_Total(&cMessageFormatListLengthK, &cMessageFormatListLengthK_1, iMessageLength, iTargetSupp, &g_uiMessageID, p_iMinSuppUnitMsg, p_iTotalMsgCount);

						//불필요한 K-1 삭제
						cMessageFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpMessageFormatList->insert(&cMessageFormatListLengthK_1);

						//K-1 비움
						cMessageFormatListLengthK_1.reset();

						//K  K-1로 복사
						cMessageFormatListLengthK_1.insert(&cMessageFormatListLengthK);

						//K 비움
						cMessageFormatListLengthK.reset();
						
						iMessageLength++;
					}
					while (cMessageFormatListLengthK_1.clMessageFormat.size());

			break;
		case 1:	//Supp_unit = message_direction
					//길이 1인 후보 Format 추출	(Field Format 1개짜리로 이루어진 Message Format 구성)
					cMessageFormatListLengthK_1.insert(p_cpFieldFormatList, &g_uiMessageID);
							
					//용의자 리스트 구성
					p_cpMessageSequenceVector->constructMessageSuspectlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//분석하는 message id list 구성
					p_cpMessageSequenceVector->constructIdentifiedMessageIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);
					//분석하는 message id list와 flow id list 구성
					//p_cpMessageSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//파일 지지도 계산
					p_cpMessageSequenceVector->yieldMessageSupport_For_MessageFormatList(&cMessageFormatListLengthK_1);
					
					//파일 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport(&cMessageFormatListLengthK_1, iTargetSupp);

					//message_total 단위의 지지도 미만 Format 삭제
					p_cpMessageSequenceVector->deleteUnderSupport_unit_Message_Direction(&cMessageFormatListLengthK_1, p_iMinSuppUnitMsg, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);
				
					//길이 K 후보 Format 추출
					iMessageLength=2;
					
					do
					{
						printf("Format num K-1: %d\n",cMessageFormatListLengthK_1.clMessageFormat.size());
						printf("Format num final: %d\n",p_cpMessageFormatList->clMessageFormat.size());

						//K-1를 사용하여 K 생성
						p_cpMessageSequenceVector->extractMessageFormatLengthK_Message_Direction(&cMessageFormatListLengthK, &cMessageFormatListLengthK_1, iMessageLength, iTargetSupp, &g_uiMessageID, p_iMinSuppUnitMsg, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);

						//불필요한 K-1 삭제
						cMessageFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpMessageFormatList->insert(&cMessageFormatListLengthK_1);

						//K-1 비움
						cMessageFormatListLengthK_1.reset();

						//K  K-1로 복사
						cMessageFormatListLengthK_1.insert(&cMessageFormatListLengthK);

						//K 비움
						cMessageFormatListLengthK.reset();
						
						iMessageLength++;
					}
					while (cMessageFormatListLengthK_1.clMessageFormat.size());
			break;
		case 2:		//Supp_unit = flow
					//길이 1인 후보 Format 추출	(Field Format 1개짜리로 이루어진 Message Format 구성)
					cMessageFormatListLengthK_1.insert(p_cpFieldFormatList, &g_uiMessageID);
							
					//용의자 리스트 구성
					p_cpMessageSequenceVector->constructMessageSuspectlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//분석하는 message id list 구성
					//p_cpMessageSequenceVector->constructIdentifiedMessageIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);
					//분석하는 message id list와 flow id list 구성
					p_cpMessageSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//파일 지지도 계산
					p_cpMessageSequenceVector->yieldMessageSupport_For_MessageFormatList(&cMessageFormatListLengthK_1);
					
					//파일 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport(&cMessageFormatListLengthK_1, iTargetSupp);

					//message_total 단위의 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport_unit_Message_Direction(&cMessageFormatListLengthK_1, p_iMinSuppUnitMsg, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);
				
					//flow 단위의 지지도 미만 Format 삭제
					p_cpMessageSequenceVector->deleteUnderSupport_unit_Flow(&cMessageFormatListLengthK_1, p_iMinSuppUnitFlow, p_iFlowCount);

					//길이 K 후보 Format 추출
					iMessageLength=2;
					
					do
					{
						printf("Format num K-1: %d\n",cMessageFormatListLengthK_1.clMessageFormat.size());
						printf("Format num final: %d\n",p_cpMessageFormatList->clMessageFormat.size());

						//K-1를 사용하여 K 생성
						p_cpMessageSequenceVector->extractMessageFormatLengthK_Message_Total_prunning_Flow(&cMessageFormatListLengthK, &cMessageFormatListLengthK_1, iMessageLength, iTargetSupp, &g_uiMessageID, p_iMinSuppUnitFlow, p_iFlowCount);

						//불필요한 K-1 삭제
						cMessageFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpMessageFormatList->insert(&cMessageFormatListLengthK_1);

						//K-1 비움
						cMessageFormatListLengthK_1.reset();

						//K  K-1로 복사
						cMessageFormatListLengthK_1.insert(&cMessageFormatListLengthK);

						//K 비움
						cMessageFormatListLengthK.reset();
						
						iMessageLength++;
					}
					while (cMessageFormatListLengthK_1.clMessageFormat.size());
			break;
		case 3:		//Supp_unit = File
					//길이 1인 후보 Format 추출	(Field Format 1개짜리로 이루어진 Message Format 구성)
					cMessageFormatListLengthK_1.insert(p_cpFieldFormatList, &g_uiMessageID);
							
					//용의자 리스트 구성
					p_cpMessageSequenceVector->constructMessageSuspectlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//분석하는 message id list 구성
					p_cpMessageSequenceVector->constructIdentifiedMessageIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);
					//분석하는 message id list와 flow id list 구성
					//p_cpMessageSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//파일 지지도 계산
					p_cpMessageSequenceVector->yieldMessageSupport_For_MessageFormatList(&cMessageFormatListLengthK_1);
					
					//파일 지지도 미만 Format 삭제
					p_cpMessageSequenceVector->deleteUnderSupport(&cMessageFormatListLengthK_1, iTargetSupp);

					//message_total 단위의 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport_unit_Message_Direction(&cMessageFormatListLengthK_1, p_iMinSuppUnitMsg, p_iDIREC_REQUESTMsgCount, p_iDIREC_RESPONSEMsgCount);
				
					//flow 단위의 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport_unit_Flow(&cMessageFormatListLengthK_1, p_iMinSuppUnitFlow, p_iFlowCount);

					//길이 K 후보 Format 추출
					iMessageLength=2;
					
					do
					{
						printf("Format num K-1: %d\n",cMessageFormatListLengthK_1.clMessageFormat.size());
						printf("Format num final: %d\n",p_cpMessageFormatList->clMessageFormat.size());

						//K-1를 사용하여 K 생성
						p_cpMessageSequenceVector->extractMessageFormatLengthK_Message_Total_prunning_File(&cMessageFormatListLengthK, &cMessageFormatListLengthK_1, iMessageLength, iTargetSupp, &g_uiMessageID, p_iMinSuppUnitMsg, p_iTotalMsgCount);

						//불필요한 K-1 삭제
						cMessageFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpMessageFormatList->insert(&cMessageFormatListLengthK_1);

						//K-1 비움
						cMessageFormatListLengthK_1.reset();

						//K  K-1로 복사
						cMessageFormatListLengthK_1.insert(&cMessageFormatListLengthK);

						//K 비움
						cMessageFormatListLengthK.reset();
						
						iMessageLength++;
					}
					while (cMessageFormatListLengthK_1.clMessageFormat.size());
			break;

			//수정중
		case 4:		//Supp_unit = Session set and Site-Specific Session set
					//길이 1인 후보 Format 추출	(Field Format 1개짜리로 이루어진 Message Format 구성)
					cMessageFormatListLengthK_1.insert(p_cpFieldFormatList, &g_uiMessageID);
							
					//용의자 리스트 구성
					p_cpMessageSequenceVector->constructMessageSuspectlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//분석하는 message id list 구성
					//p_cpMessageSequenceVector->constructIdentifiedMessageIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);
					//분석하는 message id list와 flow id list 구성
					p_cpMessageSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//파일 지지도 계산
					p_cpMessageSequenceVector->yieldMessageSupport_For_MessageFormatList(&cMessageFormatListLengthK_1);
					
					//파일 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport(&cMessageFormatListLengthK_1, iTargetSupp);

					//Session set, Site-Specific Session set 단위의 지지도 미만 Format 삭제
					Site_count=p_cpMessageSequenceVector->deleteUnderSupport_unit_AutoReEngine_forMessageFormat(p_cpSiteSpecificSessionsetSequenceVector,&cMessageFormatListLengthK_1, p_iMinSuppUnitFlow, p_iFlowCount,p_iMinSuppUnitServer);
				
					//길이 K 후보 Format 추출
					iMessageLength=2;
					
					do
					{
						printf("Format num K-1: %d\n",cMessageFormatListLengthK_1.clMessageFormat.size());
						printf("Format num final: %d\n",p_cpMessageFormatList->clMessageFormat.size());

						//K-1를 사용하여 K 생성
						p_cpMessageSequenceVector->extractMessageFormatLengthK_prunning_AutoReEngine(p_cpSiteSpecificSessionsetSequenceVector, &cMessageFormatListLengthK, &cMessageFormatListLengthK_1, iMessageLength, iTargetSupp, &g_uiMessageID, p_iMinSuppUnitFlow, p_iFlowCount,p_iMinSuppUnitServer,Site_count);

						//불필요한 K-1 삭제
						cMessageFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpMessageFormatList->insert(&cMessageFormatListLengthK_1);

						//K-1 비움
						cMessageFormatListLengthK_1.reset();

						//K  K-1로 복사
						cMessageFormatListLengthK_1.insert(&cMessageFormatListLengthK);

						//K 비움
						cMessageFormatListLengthK.reset();
						
						iMessageLength++;
					}
					while (cMessageFormatListLengthK_1.clMessageFormat.size());

			break;
		default:	//Supp_unit = message
					//길이 1인 후보 Format 추출	(Field Format 1개짜리로 이루어진 Message Format 구성)
					cMessageFormatListLengthK_1.insert(p_cpFieldFormatList, &g_uiMessageID);
							
					//용의자 리스트 구성
					p_cpMessageSequenceVector->constructMessageSuspectlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//분석하는 message id list 구성
					p_cpMessageSequenceVector->constructIdentifiedMessageIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);
					//분석하는 message id list와 flow id list 구성
					//p_cpMessageSequenceVector->constructIdentifiedMessageId_and_FlowIdlist_For_MessageFormatList(&cMessageFormatListLengthK_1);

					//파일 지지도 계산
					p_cpMessageSequenceVector->yieldMessageSupport_For_MessageFormatList(&cMessageFormatListLengthK_1);
					
					//파일 지지도 미만 Format 삭제
					//p_cpMessageSequenceVector->deleteUnderSupport(&cMessageFormatListLengthK_1, iTargetSupp);

					//message_total 단위의 지지도 미만 Format 삭제
					p_cpMessageSequenceVector->deleteUnderSupport_unit_Message_Total(&cMessageFormatListLengthK_1, p_iMinSuppUnitMsg, p_iTotalMsgCount);
				
					//길이 K 후보 Format 추출
					iMessageLength=2;
					
					do
					{
						printf("Format num K-1: %d\n",cMessageFormatListLengthK_1.clMessageFormat.size());
						printf("Format num final: %d\n",p_cpMessageFormatList->clMessageFormat.size());

						//K-1를 사용하여 K 생성
						p_cpMessageSequenceVector->extractMessageFormatLengthK_Message_Total(&cMessageFormatListLengthK, &cMessageFormatListLengthK_1, iMessageLength, iTargetSupp, &g_uiMessageID, p_iMinSuppUnitMsg, p_iTotalMsgCount);

						//불필요한 K-1 삭제
						cMessageFormatListLengthK_1.deleteBlank();

						//K-1 최종 리스트로 복사
						p_cpMessageFormatList->insert(&cMessageFormatListLengthK_1);

						//K-1 비움
						cMessageFormatListLengthK_1.reset();

						//K  K-1로 복사
						cMessageFormatListLengthK_1.insert(&cMessageFormatListLengthK);

						//K 비움
						cMessageFormatListLengthK.reset();
						
						iMessageLength++;
					}
					while (cMessageFormatListLengthK_1.clMessageFormat.size());

			break;
	}

	printf("MessageFormatCount : %d\n",p_cpMessageFormatList->clMessageFormat.size());


	cTimeChecker.endClock();

	puts("extractMessageFormat() : end");
	cTimeChecker.print();

	return p_cpMessageFormatList->clMessageFormat.size();
}
//########################################################################################
void setPositionMessageFormat(MessageFormatList* p_cpMessageFormatList, SequenceVector* p_cpMessageSequenceVector, int p_iDebugFlag)
{
	puts("setPositionMessageFormat() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpMessageSequenceVector->setPositionMessageFormat(p_cpMessageFormatList);

	cTimeChecker.endClock();

	puts("setPositionMessageFormat() : end");
	cTimeChecker.print();

	return;
}
//########################################################################################
int setHeaderMessageFormat(MessageFormatList* p_cpMessageFormatList, SequenceVector* p_cpMessageSequenceVector, int p_iDebugFlag)
{
	puts("setHeaderMessageFormat() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpMessageSequenceVector->setHeaderMessageFormat(p_cpMessageFormatList);
	printf("Total Message Format Count : %d\n", p_cpMessageFormatList->clMessageFormat.size());
	
	cTimeChecker.endClock();

	puts("setHeaderMessageFormat() : end");
	cTimeChecker.print();

	return p_cpMessageFormatList->clMessageFormat.size();
}
//########################################################################################
void extractAdditionalFieldFormatInEachMessageFormat(MessageFormatList* p_cpMessageFormatList, SequenceVector* p_cpMsgSequenceVector, int p_iDebugFlag)
{

	puts("extractAdditionalFieldFormatInEachMessageFormat() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpMsgSequenceVector->extractAdditionalFieldFormatInEachMessageFormat(p_cpMessageFormatList);
	

	
	cTimeChecker.endClock();

	puts("extractAdditionalFieldFormatInEachMessageFormat() : end");
	cTimeChecker.print();

	return;
}
//########################################################################################
void transferFormatSourceFormMessageToFlow(MessageFormatList* p_cpMessageFormatList,  SequenceVector* p_cpMessageSequenceVector, int p_iDebugFlag)
{
	puts("transferFormatSourceFormMessageToFlow() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();


	//###################################################
	//###################################################
	//###################################################
	//############ Format 소스 변경 ##################
	//###################################################
	//###################################################
	//###################################################

	p_cpMessageSequenceVector->transferSource(p_cpMessageFormatList);

	cTimeChecker.endClock();

	puts("transferFormatSourceFormMessageToFlow() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void extractMessageFormatIdSequence(SequenceVector* p_cpSequence, MessageFormatList* p_cpMessageFormatList, int p_iDebugFlag)
{
	puts("extractMessageFormatIdSequence() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//########## Format ID 시퀀스 생성 ###############
	//###################################################
	//###################################################
	//###################################################

	p_cpSequence->extractMessageFormatIdSequence(p_cpMessageFormatList);

	cTimeChecker.endClock();

	puts("extractMessageFormatIdSequence() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
int extractFlowFormat(SequenceVector* p_cpSiteSpecificSessionsetSequenceVector,FlowFormatList* p_cpFlowFormatList,  SequenceVector* p_cpFlowSequenceVector, MessageFormatList* p_cpMessageFormatList, int p_iTotalFwpFileCount, int p_iDebugFlag, int p_iMinSuppUnitFlow, int p_iFlowForwardCount, int p_iFlowBackwardCount, int p_iFlowCount, int p_iSupp_Unit,int p_iMinSuppUnitServer)
{
	puts("extractFlowFormat() : start");

	TimeChecker				cTimeChecker;
	int						iTargetSupp = 0;
	FlowFormatList			cFlowFormatListLengthK;
	FlowFormatList			cFlowFormatListLengthK_1;
	int						iFlowLength = 0;

	cTimeChecker.startClock();

	//minimum support is 2
	iTargetSupp = p_iTotalFwpFileCount - g_iDiffSupp;
	if (iTargetSupp < 2)
		iTargetSupp = 2;

	//###################################################
	//###################################################
	//###################################################
	//############## Flow Format 추출 ###################
	//###################################################
	//###################################################
	//###################################################


	switch(p_iSupp_Unit)
	{	//0 : message_total, 1 : message_direction, 2: Flow, 3 : FILE
		//0,1,2,3,default는 flowformat 추출을 위한 support 단위 flow로
		case 0:
		case 1:
		case 2:
				//길이 1인 후보 Format 추출 (Message Format 1개짜리로 이루어진 Flow Format 구성)
				cFlowFormatListLengthK_1.insert(p_cpMessageFormatList, &g_uiFlowID);

				//용의자 리스트 구성
				p_cpFlowSequenceVector->constructFlowSuspectlist_For_FlowFormatList(&cFlowFormatListLengthK_1);

				//분석하는 flow id list 구성, Message Format가 분석하는 Message list 조정
				p_cpFlowSequenceVector->constructIdentifiedFlowIdlist_For_FlowFormatList(&cFlowFormatListLengthK_1);

				//File 지지도 계산
				p_cpFlowSequenceVector->yieldFlowSupport_For_FlowFormatList(&cFlowFormatListLengthK_1);

				//File 지지도 미만 Format 삭제
				//p_cpFlowSequenceVector->deleteUnderSupport(&cFlowFormatListLengthK_1, iTargetSupp);

				//Flow 지지도 미만 Format 삭제
				p_cpFlowSequenceVector->deleteUnderSupport_unit_Flow_Total(&cFlowFormatListLengthK_1, p_iMinSuppUnitFlow, p_iFlowCount);

				//길이 K 후보 Format 추출
				iFlowLength=2;
				
				do
				{
					printf("Format num K-1: %d\n",cFlowFormatListLengthK_1.clFlowFormat.size());
					printf("Format num final: %d\n",p_cpFlowFormatList->clFlowFormat.size());
/*		if(10<=p_cpFlowFormatList->clFlowFormat.size())
					{
						//p_cpFlowFormatList->print();
						break;
					}*/

					//K-1를 사용하여 K 생성
					p_cpFlowSequenceVector->extractFlowFormatLengthK(&cFlowFormatListLengthK, &cFlowFormatListLengthK_1, iFlowLength, iTargetSupp, p_iMinSuppUnitFlow, p_iFlowCount);


					//불필요한 K-1 삭제
					cFlowFormatListLengthK_1.deleteBlank();

					//K-1 최종 리스트로 복사
					p_cpFlowFormatList->insert(&cFlowFormatListLengthK_1);

					//K-1 비움
					cFlowFormatListLengthK_1.reset();

					//K  K-1로 복사
					cFlowFormatListLengthK_1.insert(&cFlowFormatListLengthK);

					//K 비움
					cFlowFormatListLengthK.reset();
					
					iFlowLength++;
				}
				while (cFlowFormatListLengthK_1.clFlowFormat.size());

		case 4:
			//길이 1인 후보 Format 추출 (Message Format 1개짜리로 이루어진 Flow Format 구성)
				cFlowFormatListLengthK_1.insert(p_cpMessageFormatList, &g_uiFlowID);

				//용의자 리스트 구성
				p_cpFlowSequenceVector->constructFlowSuspectlist_For_FlowFormatList(&cFlowFormatListLengthK_1);

				//분석하는 flow id list 구성, Message Format가 분석하는 Message list 조정
				p_cpFlowSequenceVector->constructIdentifiedFlowIdlist_For_FlowFormatList(&cFlowFormatListLengthK_1);

				//File 지지도 계산
				p_cpFlowSequenceVector->yieldFlowSupport_For_FlowFormatList(&cFlowFormatListLengthK_1);

				//File 지지도 미만 Format 삭제
				//p_cpFlowSequenceVector->deleteUnderSupport(&cFlowFormatListLengthK_1, iTargetSupp);

				//Session,Site-SpecificSessionSet 지지도 미만 Format 삭제
				p_cpFlowSequenceVector->deleteUnderSupport_unit_AutoReEngine_forFlowFormat(p_cpSiteSpecificSessionsetSequenceVector,&cFlowFormatListLengthK_1, p_iMinSuppUnitFlow, p_iFlowCount,p_iMinSuppUnitServer);

				//길이 K 후보 Format 추출
				iFlowLength=2;
				
				do
				{
					printf("Format num K-1: %d\n",cFlowFormatListLengthK_1.clFlowFormat.size());
					printf("Format num final: %d\n",p_cpFlowFormatList->clFlowFormat.size());
/*		if(10<=p_cpFlowFormatList->clFlowFormat.size())
					{
						//p_cpFlowFormatList->print();
						break;
					}*/

					//K-1를 사용하여 K 생성
					p_cpFlowSequenceVector->extractFlowFormatLengthK_AutoReEngine(p_cpSiteSpecificSessionsetSequenceVector,&cFlowFormatListLengthK, &cFlowFormatListLengthK_1, iFlowLength, iTargetSupp, p_iMinSuppUnitFlow, p_iFlowCount,p_iMinSuppUnitServer);


					//불필요한 K-1 삭제
					cFlowFormatListLengthK_1.deleteBlank();

					//K-1 최종 리스트로 복사
					p_cpFlowFormatList->insert(&cFlowFormatListLengthK_1);

					//K-1 비움
					cFlowFormatListLengthK_1.reset();

					//K  K-1로 복사
					cFlowFormatListLengthK_1.insert(&cFlowFormatListLengthK);

					//K 비움
					cFlowFormatListLengthK.reset();
					
					iFlowLength++;
				}
				while (cFlowFormatListLengthK_1.clFlowFormat.size());

			break;
		default :
			break;

	}

	printf("FlowFormatCount : %d\n",p_cpFlowFormatList->clFlowFormat.size());

	cTimeChecker.endClock();

	puts("extractFlowFormat() : end");
	cTimeChecker.print();

	return p_cpFlowFormatList->clFlowFormat.size();
}
//########################################################################################
void setPositionFlowFormat(FlowFormatList* p_cpFlowFormatList, SequenceVector* p_cpMessageSequenceVector, int p_iDebugFlag)
{
	puts("setPositionFlowFormat() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpMessageSequenceVector->setPositionFlowFormat(p_cpFlowFormatList);

	cTimeChecker.endClock();

	puts("setPositionFlowFormat() : end");
	cTimeChecker.print();

	return;
}
//########################################################################################
int setHeaderFlowFormat(FlowFormatList* p_cpFlowFormatList, SequenceVector* p_cpMessageSequenceVector, int p_iDebugFlag)
{
	puts("setHeaderFlowFormat() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpMessageSequenceVector->setHeaderFlowFormat(p_cpFlowFormatList);
	printf("Total Flow Format Count : %d\n", p_cpFlowFormatList->clFlowFormat.size());
	
	cTimeChecker.endClock();

	puts("setHeaderFlowFormat() : end");
	cTimeChecker.print();

	return p_cpFlowFormatList->clFlowFormat.size();
}
//########################################################################################
void setCompletenessFlowFormat(FlowFormatList* p_cpFlowFormatList, SequenceVector* p_cpFlowSequenceVector, FlowHash* p_cpFlowHash, int p_iDebugFlag)
{
	puts("setCompletenessFlowFormat() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpFlowSequenceVector->setCompletenessFlowFormat(p_cpFlowFormatList, &g_cFlowVolume, &g_cExclusiveFlowVolume, p_cpFlowHash);

	cTimeChecker.endClock();

	puts("setCompletenessFlowFormat() : end");
	cTimeChecker.print();

	return;
}

//########################################################################################
int extractRemnantMessageSequence(SequenceVector* p_cpDstMsgSequenceVector, SequenceVector* p_cpSrcMsgSequenceVector, FieldFormatList* p_cpFieldFormatList, int p_iDebugFlag, int * p_iRemnant_DIREC_REQUESTMsgCount, int *p_iRemnant_DIREC_RESPONSEMsgCount)
{
	puts("extractRemnantMessageSequence() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();

	p_cpDstMsgSequenceVector->extractRemnantMessageSequence(p_cpSrcMsgSequenceVector, p_cpFieldFormatList, p_iRemnant_DIREC_REQUESTMsgCount, p_iRemnant_DIREC_RESPONSEMsgCount);
	printf("Total Remnant Message Sequence Count : %d\n", p_cpDstMsgSequenceVector->cvSequenceVector.size());

	cTimeChecker.endClock();

	puts("extractMessageSequence() : end");
	cTimeChecker.print();

	return p_cpDstMsgSequenceVector->cvSequenceVector.size();

}
//########################################################################################

void extractFieldFormatIdSequenceForRemnantFieldFormat(SequenceVector* p_cpSequence, FieldFormatList* p_cpFieldFormatList, int p_iDebugFlag)
{
	puts("extractFieldFormatIdSequenceForRemnantFieldFormat() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();


	//###################################################
	//###################################################
	//###################################################
	//########## Format ID 시퀀스 생성 ###############
	//###################################################
	//###################################################
	//###################################################

	p_cpSequence->extractFieldFormatIdSequenceForRemnantFieldFormat(p_cpFieldFormatList);

	cTimeChecker.endClock();

	puts("extractFieldFormatIdSequence2() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void setMinOffsetMaxDepthPositionVarFieldFormat(FieldFormatList* p_cpFieldFormatList,int p_iDebugFlag)
{
	
	puts("setMinOffsetMaxOffsetFieldFormat() : start");

	TimeChecker				cTimeChecker;

	cTimeChecker.startClock();


	//###################################################
	//###################################################
	//###################################################
	//########## MinOffsetMaxDepth setting ###############
	//###################################################
	//###################################################
	//###################################################

	p_cpFieldFormatList->setMinOffsetMaxDepthPositionVarFieldFormat();

	cTimeChecker.endClock();

	puts("setMinOffsetMaxOffsetFieldFormat() : end");
	cTimeChecker.print();
	
	return;

}
//#################################################################################################################
int constructFSM(SequenceVector* p_cpFlowSequenceVector, MessageFormatList* p_cpMessageFormatList, FSMManager* p_cpFSMManager, SequenceVector* p_cpMessageSequenceVector)
{
	puts("constructFSM() : start");

	TimeChecker				cTimeChecker;

	int iNumTransition;

	iNumTransition = p_cpFSMManager->constructFSM(p_cpFlowSequenceVector, p_cpMessageFormatList, p_cpMessageSequenceVector);

	printf("Transition Count : %d\n", iNumTransition);

	puts("constructFSM() : end");
	cTimeChecker.print();
	
	return iNumTransition;
}
//#################################################################################################################
void setCompletenessMessageFormat(SequenceVector* p_cpFlowSequenceVector, SequenceVector* p_cpMessageSequenceVector, MessageFormatList* p_cpMessageFormatList)
{
	vector<Sequence>::iterator		itSequence;
	list<FormatId>::iterator		itMessageID;
	list<MessageFormat>::iterator	itMessageFormat;


	for (itSequence = p_cpFlowSequenceVector->cvSequenceVector.begin(); itSequence != p_cpFlowSequenceVector->cvSequenceVector.end(); ++itSequence)
	{
		itMessageID = itSequence->clFormatIdListFinal.begin();
		while (itMessageID !=itSequence->clFormatIdListFinal.end())
		{
			for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
			{
				if (itMessageID->uiFormatID == itMessageFormat->uiMessageFormatID)
				{
					itMessageFormat->uiCoverdMessage++;
					p_cpMessageFormatList->uiTotalCoverdMessage++;
				}
			}
			itMessageID++;
		}
	}

	for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
	{
		itMessageFormat->dCoverage = (double)itMessageFormat->uiCoverdMessage / p_cpMessageSequenceVector->cvSequenceVector.size();
	}

	p_cpMessageFormatList->dTotalCoverage = p_cpMessageFormatList->uiTotalCoverdMessage / p_cpMessageSequenceVector->cvSequenceVector.size();

	return;
}
//########################################################################################
void printFormatListXML(char* p_cpFileName, FieldFormatList* p_cpFieldFormatList, MessageFormatList* p_cpMessageFormatList, FlowFormatList* p_cpFlowFormatList)
{
	puts("printFormatListXML() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpFieldFormat = NULL;
	xmlNodePtr xnpMessageFormat = NULL;
	xmlNodePtr xnpFlowFormat = NULL;
//	xmlTextWriterPtr xtwpWriter = NULL;


	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	int iIndex;

	char bufValue[100] = {0,};
	int iIndexValue;

//	int iFieldFormatID=1;
//	int iMessageFormatID=1;
//	int iFlowFormatID=1;

	list<FieldFormat>::iterator itFieldFormat;
	list<FieldFormat>::iterator itFieldFormat2;
	list<MessageFormat>::iterator itMessageFormat;
	list<FlowFormat>::iterator itFlowFormat;
	vector<Content>::iterator	itContent;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//################# printFormatListXML ##################
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //1.0 버전으로 생성

	
	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"AutoReEngine");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"format_list");

	xmlDocSetRootElement(xdpDoc, xnpRoot); 

		//###################################################
		//Field Format List 설정
		//###################################################

//		xnpCurTemp = xnpCur;
		xnpFieldFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"fieldformat_list", NULL);

		sprintf(buf, "%d", p_cpFieldFormatList->clFieldFormat.size());
		xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"number", (xmlChar*)buf);



			//###################################################
			//Field Format 설정
			//###################################################
			
//			xnpCurTemp = xnpCur;
			for (itFieldFormat = p_cpFieldFormatList->clFieldFormat.begin(); itFieldFormat != p_cpFieldFormatList->clFieldFormat.end(); ++itFieldFormat)
			{
				xnpCur = xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"fieldformat", NULL);

				sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);

				sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
										,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
										,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
										,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"type", (xmlChar*)buf);

				sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
									,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"direction", (xmlChar*)buf);

//				itFieldFormat->cContent.cContentHeader.extractCharP(buf);
//				xapCur = xmlNewProp(xnpCur, (xmlChar*)"header", (xmlChar*)buf);


					//Field Format의 자식 element 설정


					itFieldFormat->cContent.extractCharP(buf);
					xmlNewChild(xnpCur, NULL, (xmlChar*)"value", (xmlChar*)buf);

					if(itFieldFormat->cvAdditionalContentVector.size())
					{
						vector<Content>::iterator	itContent;

						for(itContent = itFieldFormat->cvAdditionalContentVector.begin() ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent)
						{
							itContent->extractCharP(buf);
							xmlNewChild(xnpCur, NULL, (xmlChar*)"value", (xmlChar*)buf);
						}
					}
					sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
					xmlNewChild(xnpCur, NULL, (xmlChar*)"offset", (xmlChar*)buf);

					sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
					xmlNewChild(xnpCur, NULL, (xmlChar*)"depth", (xmlChar*)buf);

					sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
					xmlNewChild(xnpCur, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
				
					sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
					xmlNewChild(xnpCur, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

					if (itFieldFormat->uiSemanticsCode)
					{
						sprintf(buf, "%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
												,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
												,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
												,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
												,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
												,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
						xmlNewChild(xnpCur, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
					}
			}			

		//###################################################
		//Message Format List 설정
		//###################################################
		xnpMessageFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"messageformat_list", NULL);
		sprintf(buf, "%d", p_cpMessageFormatList->clMessageFormat.size());
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"number", (xmlChar*)buf);
		sprintf(buf, "%.2lf%%", p_cpMessageFormatList->dTotalCoverage*100);
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"coverage", (xmlChar*)buf);
		sprintf(buf, "%u", p_cpMessageFormatList->uiTotalCoverdMessage);
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"NumOfCoveredMessage", (xmlChar*)buf);


			//###################################################
			//Message Format 설정
			//###################################################
			
//			xnpCurTemp = xnpCur;
			for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
			{
				xnpCur = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"messageformat", NULL);

				sprintf(buf, "%u", itMessageFormat->uiMessageFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);
				sprintf(buf, "%s%s", itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_REQUEST ? "request" : ""
									,itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"direction", (xmlChar*)buf);

				itMessageFormat->cMessageHeader.extractCharP(buf);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"header", (xmlChar*)buf);
				sprintf(buf, "%d", itMessageFormat->clFieldFormat.size()+itMessageFormat->clDynamicFieldFormat.size());
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"number-of-fieldformat", (xmlChar*)buf);
				sprintf(buf, "%.2lf%%", itMessageFormat->dCoverage*100);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"coverage", (xmlChar*)buf);
				sprintf(buf, "%u", itMessageFormat->uiCoverdMessage);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"NumOfCoveredMessage", (xmlChar*)buf);

				//Message Format의 자식 Field Format 설정
				itFieldFormat = itMessageFormat->clFieldFormat.begin();
				itFieldFormat2 = itMessageFormat->clDynamicFieldFormat.begin();
				iIndex=0;
				
				while ((itFieldFormat != itMessageFormat->clFieldFormat.end()) && (itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
				{
					if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat > iIndex)
					{
						xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
						sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
												,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
												,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
												,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
						sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
							
							//Field Format의 자식 element 설정
							itFieldFormat->cContent.extractCharP(buf);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

							if(itFieldFormat->cvAdditionalContentVector.size())
							{
								for(itContent = itFieldFormat->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
								{
									itContent->extractCharP(buf);
									//sprintf(bufValue, "%s%d", "value",iIndexValue);
									//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								}
							}
							sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);
							
							if(itFieldFormat->uiType & DFV_TYPE)
							{
								sprintf(buf, "%u", itFieldFormat->cDynamicFieldPosition.uiMinLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat->cDynamicFieldPosition.uiMaxLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);
							}
							else
							{
								sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);
							}


							if (itFieldFormat->uiSemanticsCode)
							{
								sprintf(buf, "%s%s%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
														,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
														,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
														,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
														,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
														,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
							}
						
						iIndex++;
						itFieldFormat++;
					}
					else if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat == iIndex)
					{
						xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
						sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
																	,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);

						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
												,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
												,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
												,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
						sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
							
							//Field Format의 자식 element 설정

							if (!(itFieldFormat2->uiType & GAP_TYPE))
							{
								if(itFieldFormat2->uiType & DFV_TYPE)
								{
									itFieldFormat2->cContent.extractCharP(buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

									for(itContent = itFieldFormat2->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat2->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
									{
										itContent->extractCharP(buf);
										//sprintf(bufValue, "%s%d", "value",iIndexValue);
										//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}
								}
								sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
							}


							if (itFieldFormat2->uiSemanticsCode)
							{
								sprintf(buf, "%s%s%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
														,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
														,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
														,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
														,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
														,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
							}

						itFieldFormat2++;
					}
					else
						g_err((char*)"printFormatListXML() : In messageformat node, field format parsing error");
				}


				if ((itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
				{
					xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
					sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
																,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
					sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
											,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
											,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
											,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
					sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
										,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
						
						//Field Format의 자식 element 설정

						if (!(itFieldFormat2->uiType & GAP_TYPE))
						{
							if(itFieldFormat2->uiType & DFV_TYPE)
							{
								itFieldFormat2->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								
								for(itContent = itFieldFormat2->cvAdditionalContentVector.begin(), iIndexValue=2; itContent != itFieldFormat2->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
								{
									itContent->extractCharP(buf);
									//sprintf(bufValue, "%s%d", "value",iIndexValue);
									//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								}
							}
							sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
						
							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
						}


						if (itFieldFormat2->uiSemanticsCode)
						{
							sprintf(buf, "%s%s%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
													,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
													,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
						}
				}
				if ((itFieldFormat != itMessageFormat->clFieldFormat.end()))
				{
					xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
					sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
					sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
											,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
											,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
											,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
					sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
										,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
						
						//Field Format의 자식 element 설정
						itFieldFormat->cContent.extractCharP(buf);
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

						if(itFieldFormat->cvAdditionalContentVector.size())
						{
							for(itContent = itFieldFormat->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
							{
								itContent->extractCharP(buf);
								//sprintf(bufValue, "%s%d", "value",iIndexValue);
								//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
							}
						}
						sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

						sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

						sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
					
						sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);


						if (itFieldFormat->uiSemanticsCode)
						{
							sprintf(buf, "%s%s%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
													,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
													,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
													,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
													,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
													,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
						}
				}
			}		


		//###################################################
		//Flow Format List 설정
		//###################################################
		xnpFlowFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"flowformat_list", NULL);
		sprintf(buf, "%d", p_cpFlowFormatList->clFlowFormat.size());
		xapCur = xmlNewProp(xnpFlowFormat, (xmlChar*)"number", (xmlChar*)buf);
			//###################################################
			//Flow Format 설정
			//###################################################
			for (itFlowFormat = p_cpFlowFormatList->clFlowFormat.begin(); itFlowFormat != p_cpFlowFormatList->clFlowFormat.end(); ++itFlowFormat)
			{
				xnpCur = xmlNewChild(xnpFlowFormat, NULL, (xmlChar*)"flowformat", NULL);

				sprintf(buf, "%u", itFlowFormat->uiFlowFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);
				sprintf(buf, "%d", itFlowFormat->clMessageFormat.size());
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"number-of-messageformat", (xmlChar*)buf);

					//Flow Format의 자식 Message Format 설정
					for (itMessageFormat = itFlowFormat->clMessageFormat.begin(); itMessageFormat != itFlowFormat->clMessageFormat.end(); ++itMessageFormat)
					{
						xnpMessageFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"messageformat", NULL);
						sprintf(buf, "%u", itMessageFormat->uiMessageFormatIDArrange);
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s", itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"direction", (xmlChar*)buf);
						itMessageFormat->cMessageHeader.extractCharP(buf);
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"header", (xmlChar*)buf);
						sprintf(buf, "%d", itMessageFormat->clFieldFormat.size()+itMessageFormat->clDynamicFieldFormat.size());
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"number-of-fieldformat", (xmlChar*)buf);

						//Message Format의 자식 Field Format 설정
						itFieldFormat = itMessageFormat->clFieldFormat.begin();
						itFieldFormat2 = itMessageFormat->clDynamicFieldFormat.begin();
						iIndex=0;

						while ((itFieldFormat != itMessageFormat->clFieldFormat.end()) && (itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
						{
							if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat > iIndex)
							{
								xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
								sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
								sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
														,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
														,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
														,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
								sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
													,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
									
									//Field Format의 자식 element 설정
									itFieldFormat->cContent.extractCharP(buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

									if(itFieldFormat->cvAdditionalContentVector.size())
									{
										vector<Content>::iterator	itContent;

										for(itContent = itFieldFormat->cvAdditionalContentVector.begin() ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent)
										{
											itContent->extractCharP(buf);
											xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
										}
									}
									sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
								
									sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);


									if (itFieldFormat->uiSemanticsCode)
									{
										sprintf(buf, "%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
																,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
																,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
																,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
																,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
																,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
									}
								
								iIndex++;
								itFieldFormat++;
							}
							else if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat == iIndex)
							{
								xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
								sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
											,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
								sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
														,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
														,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
														,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
								sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
													,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
									
									//Field Format의 자식 element 설정

									if (!(itFieldFormat2->uiType & GAP_TYPE))
									{
										if(itFieldFormat2->uiType & DFV_TYPE)
										{
											itFieldFormat2->cContent.extractCharP(buf);
											xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

											itFieldFormat2->cvAdditionalContentVector.front().extractCharP(buf);
											xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
										}
										sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

										sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

										sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
									
										sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

										sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
									}
									

									if (itFieldFormat2->uiSemanticsCode)
									{
										sprintf(buf, "%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
																,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
																,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
																,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
																,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
																,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
									}

								itFieldFormat2++;
							}
							else
								g_err((char*)"printFormatListXML() : In messageformat node, field format parsing error");
						}
						if ((itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
						{
							xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
							sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
											,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
													,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
													,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
													,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
							sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
												,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
								
								//Field Format의 자식 element 설정

								if (!(itFieldFormat2->uiType & GAP_TYPE))
								{
									if(itFieldFormat2->uiType & DFV_TYPE)
									{
										itFieldFormat2->cContent.extractCharP(buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

										itFieldFormat2->cvAdditionalContentVector.front().extractCharP(buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}
									sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
								}
								

								if (itFieldFormat2->uiSemanticsCode)
								{
									sprintf(buf, "%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
															,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
															,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
															,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
															,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
															,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
								}
						}
						if ((itFieldFormat != itMessageFormat->clFieldFormat.end()))
						{
							xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
							sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
													,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
													,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
													,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
							sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
												,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
								
								//Field Format의 자식 element 설정
								itFieldFormat->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

								if(itFieldFormat->cvAdditionalContentVector.size())
								{
									vector<Content>::iterator	itContent;

									for(itContent = itFieldFormat->cvAdditionalContentVector.begin() ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent)
									{
										itContent->extractCharP(buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}
								}
								sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);


								if (itFieldFormat->uiSemanticsCode)
								{
									sprintf(buf, "%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
															,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
															,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
															,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
															,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
															,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
								}
						}
					}
			}


	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "utf-8", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printFormatListXML() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void printFieldFormatListXML(char* p_cpFileName, FieldFormatList* p_cpFieldFormatList)
{
	puts("printFieldFormatListXML() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpFieldFormat = NULL;
	xmlNodePtr xnpMessageFormat = NULL;
	xmlNodePtr xnpFlowFormat = NULL;
//	xmlTextWriterPtr xtwpWriter = NULL;


	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	int iIndex;

	char bufValue[100] = {0,};
	int iIndexValue;

//	int iFieldFormatID=1;
//	int iMessageFormatID=1;
//	int iFlowFormatID=1;

	list<FieldFormat>::iterator itFieldFormat;
	list<FieldFormat>::iterator itFieldFormat2;
	list<MessageFormat>::iterator itMessageFormat;
	list<FlowFormat>::iterator itFlowFormat;
	vector<Content>::iterator	itContent;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//################# printFormatListXML ##################
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //1.0 버전으로 생성

	
	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"AutoReEngine");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"fieldformat_list");
	sprintf(buf, "%d", g_iDIREC_REQUESTMsgCount+g_iDIREC_RESPONSEMsgCount);
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"NumOfMessage", (xmlChar*)buf);

	xmlDocSetRootElement(xdpDoc, xnpRoot); 

		//###################################################
		//Field Format List 설정
		//###################################################

//		xnpCurTemp = xnpCur;
		xnpFieldFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"fieldformat_list", NULL);

		sprintf(buf, "%d", p_cpFieldFormatList->clFieldFormat.size());
		xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"number", (xmlChar*)buf);



			//###################################################
			//Field Format 설정
			//###################################################
			
//			xnpCurTemp = xnpCur;
			for (itFieldFormat = p_cpFieldFormatList->clFieldFormat.begin(); itFieldFormat != p_cpFieldFormatList->clFieldFormat.end(); ++itFieldFormat)
			{
				xnpCur = xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"fieldformat", NULL);

				sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);

				sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
										,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
										,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
										,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"type", (xmlChar*)buf);

				sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
									,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"direction", (xmlChar*)buf);


//				itFieldFormat->cContent.cContentHeader.extractCharP(buf);
//				xapCur = xmlNewProp(xnpCur, (xmlChar*)"header", (xmlChar*)buf);


					//Field Format의 자식 element 설정


					itFieldFormat->cContent.extractCharP(buf);
					xmlNewChild(xnpCur, NULL, (xmlChar*)"value", (xmlChar*)buf);

					if(itFieldFormat->cvAdditionalContentVector.size())
					{
						vector<Content>::iterator	itContent;

						for(itContent = itFieldFormat->cvAdditionalContentVector.begin() ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent)
						{
							itContent->extractCharP(buf);
							xmlNewChild(xnpCur, NULL, (xmlChar*)"value", (xmlChar*)buf);
						}
					}


					sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
					xmlNewChild(xnpCur, NULL, (xmlChar*)"offset", (xmlChar*)buf);

					sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
					xmlNewChild(xnpCur, NULL, (xmlChar*)"depth", (xmlChar*)buf);

					sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
					xmlNewChild(xnpCur, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
				
					sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
					xmlNewChild(xnpCur, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

					sprintf(buf,"%.3lf",itFieldFormat->dSupp_unit_flow);
					xmlNewChild(xnpCur,NULL,(xmlChar*)"session_support", (xmlChar*)buf);

					sprintf(buf,"%.3lf",itFieldFormat->dSupp_unit_server);
					xmlNewChild(xnpCur,NULL,(xmlChar*)"sessionset_support", (xmlChar*)buf);

					sprintf(buf,"%.3lf",itFieldFormat->NormVar);
					xmlNewChild(xnpCur,NULL,(xmlChar*)"position_var", (xmlChar*)buf);

					if (itFieldFormat->uiSemanticsCode)
					{
						sprintf(buf, "%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
												,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
												,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
												,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
												,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
												,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
						xmlNewChild(xnpCur, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
					}
			}			

		
		




	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "utf-8", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printFieldFormatListXML() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void printMessageFormatListXML(char* p_cpFileName, FieldFormatList* p_cpFieldFormatList, MessageFormatList* p_cpMessageFormatList)
{
	puts("printMessageFormatListXML() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpFieldFormat = NULL;
	xmlNodePtr xnpMessageFormat = NULL;
	xmlNodePtr xnpFlowFormat = NULL;
//	xmlTextWriterPtr xtwpWriter = NULL;


	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	int iIndex;

	char bufValue[100] = {0,};
	int iIndexValue;

//	int iFieldFormatID=1;
//	int iMessageFormatID=1;
//	int iFlowFormatID=1;

	list<FieldFormat>::iterator itFieldFormat;
	list<FieldFormat>::iterator itFieldFormat2;
	list<MessageFormat>::iterator itMessageFormat;
	list<FlowFormat>::iterator itFlowFormat;
	vector<Content>::iterator	itContent;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//################# printFormatListXML ##################
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //1.0 버전으로 생성

	
	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"AutoReEngine");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"messageformat_list");
	sprintf(buf, "%d", g_iDIREC_REQUESTMsgCount+g_iDIREC_RESPONSEMsgCount);
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"NumOfMessage", (xmlChar*)buf);

	
	xmlDocSetRootElement(xdpDoc, xnpRoot); 

		//###################################################
		//Field Format List 설정
		//###################################################

//		xnpCurTemp = xnpCur;
		



			

		//###################################################
		//Message Format List 설정
		//###################################################
		xnpMessageFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"messageformat_list", NULL);
		sprintf(buf, "%d", p_cpMessageFormatList->clMessageFormat.size());
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"number", (xmlChar*)buf);
		sprintf(buf, "%.2lf%%", p_cpMessageFormatList->dTotalCoverage*100);
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"coverage", (xmlChar*)buf);
		sprintf(buf, "%u", p_cpMessageFormatList->uiTotalCoverdMessage);
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"NumOfCoveredMessage", (xmlChar*)buf);


			//###################################################
			//Message Format 설정
			//###################################################
			
//			xnpCurTemp = xnpCur;
		for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
			{
				xnpCur = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"messageformat", NULL);

				sprintf(buf, "%u", itMessageFormat->uiMessageFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);
				sprintf(buf, "%s%s", itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_REQUEST ? "request" : ""
									,itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"direction", (xmlChar*)buf);

				itMessageFormat->cMessageHeader.extractCharP(buf);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"header", (xmlChar*)buf);
				sprintf(buf, "%d", itMessageFormat->clFieldFormat.size()+itMessageFormat->clDynamicFieldFormat.size());
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"number-of-fieldformat", (xmlChar*)buf);
				sprintf(buf, "%.2lf%%", itMessageFormat->dCoverage*100);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"coverage", (xmlChar*)buf);
				sprintf(buf, "%u", itMessageFormat->uiCoverdMessage);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"NumOfCoveredMessage", (xmlChar*)buf);

				sprintf(buf,"%.3lf",itMessageFormat->dSupp_unit_flow);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"session_support", (xmlChar*)buf);

				sprintf(buf,"%.3lf",itMessageFormat->dSupp_unit_server);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"sessionset_support", (xmlChar*)buf);

				//Message Format의 자식 Field Format 설정
				itFieldFormat = itMessageFormat->clFieldFormat.begin();
				itFieldFormat2 = itMessageFormat->clDynamicFieldFormat.begin();
				iIndex=0;
				
				while ((itFieldFormat != itMessageFormat->clFieldFormat.end()) && (itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
				{
					if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat > iIndex)
					{
						xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
						sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
												,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
												,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
												,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
						sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);


							
							//Field Format의 자식 element 설정
							itFieldFormat->cContent.extractCharP(buf);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

							if(itFieldFormat->cvAdditionalContentVector.size())
							{
								for(itContent = itFieldFormat->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
								{
									itContent->extractCharP(buf);
									//sprintf(bufValue, "%s%d", "value",iIndexValue);
									//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								}
							}


							sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);
							
							if(itFieldFormat->uiType & DFV_TYPE)
							{
								sprintf(buf, "%u", itFieldFormat->cDynamicFieldPosition.uiMinLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat->cDynamicFieldPosition.uiMaxLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);
							}
							else
							{
								sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);
							}


							if (itFieldFormat->uiSemanticsCode)
							{
								sprintf(buf, "%s%s%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
														,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
														,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
														,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
														,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
														,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
							}
						
						iIndex++;
						itFieldFormat++;
					}
					else if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat == iIndex)
					{
						xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
						sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
																	,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);

						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
												,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
												,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
												,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
						sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
							
							//Field Format의 자식 element 설정

							if (!(itFieldFormat2->uiType & GAP_TYPE))
							{
								if(itFieldFormat2->uiType & DFV_TYPE)
								{
									itFieldFormat2->cContent.extractCharP(buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

									for(itContent = itFieldFormat2->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat2->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
									{
										itContent->extractCharP(buf);
										//sprintf(bufValue, "%s%d", "value",iIndexValue);
										//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}
								}


								sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
							}


							if (itFieldFormat2->uiSemanticsCode)
							{
								sprintf(buf, "%s%s%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
														,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
														,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
														,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
														,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
														,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
							}

						itFieldFormat2++;
					}
					else
						g_err((char*)"printFormatListXML() : In messageformat node, field format parsing error");
				}


				if ((itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
				{
					xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
					sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
																,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
					sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
											,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
											,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
											,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
					sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
										,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
						
						//Field Format의 자식 element 설정

						if (!(itFieldFormat2->uiType & GAP_TYPE))
						{
							if(itFieldFormat2->uiType & DFV_TYPE)
							{
								itFieldFormat2->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								
								for(itContent = itFieldFormat2->cvAdditionalContentVector.begin(), iIndexValue=2; itContent != itFieldFormat2->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
								{
									itContent->extractCharP(buf);
									//sprintf(bufValue, "%s%d", "value",iIndexValue);
									//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								}
							}


							sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
						
							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
						}


						if (itFieldFormat2->uiSemanticsCode)
						{
							sprintf(buf, "%s%s%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
													,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
													,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
						}
				}
				if ((itFieldFormat != itMessageFormat->clFieldFormat.end()))
				{
					xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
					sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
					sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
											,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
											,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
											,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
					sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
										,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
						
						//Field Format의 자식 element 설정
						itFieldFormat->cContent.extractCharP(buf);
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

						if(itFieldFormat->cvAdditionalContentVector.size())
						{
							for(itContent = itFieldFormat->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
							{
								itContent->extractCharP(buf);
								//sprintf(bufValue, "%s%d", "value",iIndexValue);
								//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
							}
						}

						sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

						sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

						sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
					
						sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
						xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);


						if (itFieldFormat->uiSemanticsCode)
						{
							sprintf(buf, "%s%s%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
													,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
													,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
													,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
													,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
													,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
						}
				}
			}		



		


	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "utf-8", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printMessageFormatListXML() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void printFlowFormatListXML(char* p_cpFileName,FieldFormatList* p_cpFieldFormatList, MessageFormatList* p_cpMessageFormatList, FlowFormatList* p_cpFlowFormatList)
{
	puts("printFlowFormatListXML() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpFieldFormat = NULL;
	xmlNodePtr xnpMessageFormat = NULL;
	xmlNodePtr xnpFlowFormat = NULL;
//	xmlTextWriterPtr xtwpWriter = NULL;


	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	int iIndex;

	char bufValue[100] = {0,};
	int iIndexValue;

//	int iFieldFormatID=1;
//	int iMessageFormatID=1;
//	int iFlowFormatID=1;

	list<FieldFormat>::iterator itFieldFormat;
	list<FieldFormat>::iterator itFieldFormat2;
	list<MessageFormat>::iterator itMessageFormat;
	list<FlowFormat>::iterator itFlowFormat;
	vector<Content>::iterator	itContent;

	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//################# printFormatListXML ##################
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //1.0 버전으로 생성

	
	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"AutoReEngine");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"flowformat_list");
	sprintf(buf, "%d", g_iDIREC_REQUESTMsgCount+g_iDIREC_RESPONSEMsgCount);
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"NumOfMessage", (xmlChar*)buf);

	xmlDocSetRootElement(xdpDoc, xnpRoot); 

		

		//###################################################
		//Flow Format List 설정
		//###################################################
		xnpFlowFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"flowformat_list", NULL);
		sprintf(buf, "%d", p_cpFlowFormatList->clFlowFormat.size());
		xapCur = xmlNewProp(xnpFlowFormat, (xmlChar*)"number", (xmlChar*)buf);
			//###################################################
			//Flow Format 설정
			//###################################################
			for (itFlowFormat = p_cpFlowFormatList->clFlowFormat.begin(); itFlowFormat != p_cpFlowFormatList->clFlowFormat.end(); ++itFlowFormat)
			{
				xnpCur = xmlNewChild(xnpFlowFormat, NULL, (xmlChar*)"flowformat", NULL);

				sprintf(buf, "%u", itFlowFormat->uiFlowFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);
				sprintf(buf, "%d", itFlowFormat->clMessageFormat.size());
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"number-of-messageformat", (xmlChar*)buf);

				sprintf(buf,"%.3lf",itFlowFormat->dSupp_unit_flow);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"session_support", (xmlChar*)buf);

				sprintf(buf,"%.3lf",itFlowFormat->dSupp_unit_server);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"sessionset_support", (xmlChar*)buf);

					//Flow Format의 자식 Message Format 설정
					for (itMessageFormat = itFlowFormat->clMessageFormat.begin(); itMessageFormat != itFlowFormat->clMessageFormat.end(); ++itMessageFormat)
					{
						xnpMessageFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"messageformat", NULL);
						sprintf(buf, "%u", itMessageFormat->uiMessageFormatIDArrange);
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s", itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"direction", (xmlChar*)buf);
						itMessageFormat->cMessageHeader.extractCharP(buf);
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"header", (xmlChar*)buf);
						sprintf(buf, "%d", itMessageFormat->clFieldFormat.size()+itMessageFormat->clDynamicFieldFormat.size());
						xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"number-of-fieldformat", (xmlChar*)buf);



						//Message Format의 자식 Field Format 설정
						itFieldFormat = itMessageFormat->clFieldFormat.begin();
						itFieldFormat2 = itMessageFormat->clDynamicFieldFormat.begin();
						iIndex=0;

						while ((itFieldFormat != itMessageFormat->clFieldFormat.end()) && (itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
						{
							if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat > iIndex)
							{
								xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
								sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
								sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
														,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
														,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
														,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
								sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
													,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
									
									//Field Format의 자식 element 설정
									itFieldFormat->cContent.extractCharP(buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

									if(itFieldFormat->cvAdditionalContentVector.size())
									{
										vector<Content>::iterator	itContent;

										for(itContent = itFieldFormat->cvAdditionalContentVector.begin() ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent)
										{
											itContent->extractCharP(buf);
											xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
										}
									}


									sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
								
									sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);


									if (itFieldFormat->uiSemanticsCode)
									{
										sprintf(buf, "%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
																,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
																,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
																,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
																,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
																,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
									}
								
								iIndex++;
								itFieldFormat++;
							}
							else if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat == iIndex)
							{
								xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
								sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
											,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
								sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
														,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
														,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
														,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
								sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
													,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
								xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
									
									//Field Format의 자식 element 설정

									if (!(itFieldFormat2->uiType & GAP_TYPE))
									{
										if(itFieldFormat2->uiType & DFV_TYPE)
										{
											itFieldFormat2->cContent.extractCharP(buf);
											xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

											itFieldFormat2->cvAdditionalContentVector.front().extractCharP(buf);
											xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
										}


										sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

										sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

										sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
									
										sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

										sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
									}
									

									if (itFieldFormat2->uiSemanticsCode)
									{
										sprintf(buf, "%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
																,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
																,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
																,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
																,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
																,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
									}

								itFieldFormat2++;
							}
							else
								g_err((char*)"printFormatListXML() : In messageformat node, field format parsing error");
						}
						if ((itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
						{
							xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
							sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
											,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
													,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
													,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
													,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
							sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
												,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
								
								//Field Format의 자식 element 설정

								if (!(itFieldFormat2->uiType & GAP_TYPE))
								{
									if(itFieldFormat2->uiType & DFV_TYPE)
									{
										itFieldFormat2->cContent.extractCharP(buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

										itFieldFormat2->cvAdditionalContentVector.front().extractCharP(buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}



									sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

									sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
								}
								

								if (itFieldFormat2->uiSemanticsCode)
								{
									sprintf(buf, "%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
															,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
															,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
															,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
															,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
															,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
								}
						}
						if ((itFieldFormat != itMessageFormat->clFieldFormat.end()))
						{
							xnpFieldFormat = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"fieldformat", NULL);
							sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
													,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
													,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
													,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
							sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
												,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);
								
								//Field Format의 자식 element 설정
								itFieldFormat->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

								if(itFieldFormat->cvAdditionalContentVector.size())
								{
									vector<Content>::iterator	itContent;

									for(itContent = itFieldFormat->cvAdditionalContentVector.begin() ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent)
									{
										itContent->extractCharP(buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}
								}


								sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);
							
								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth - itFieldFormat->cContent.uiOffset + 1);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);


								if (itFieldFormat->uiSemanticsCode)
								{
									sprintf(buf, "%s%s%s%s", itFieldFormat->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
															,itFieldFormat->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
															,itFieldFormat->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
															,itFieldFormat->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
															,itFieldFormat->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
															,itFieldFormat->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
								}
						}
					}
			}


	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "utf-8", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printFlowFormatListXML() : end");
	cTimeChecker.print();
	
	return;
}
//########################################################################################
void printFsmXML(char* p_cpFileName, FSMManager* p_cpFSMManager)
{
	puts("printFsmXML()() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;		//depth 2
	xmlNodePtr xnpCur2 = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpState = NULL;
	xmlNodePtr xnpTransition = NULL;
//	xmlTextWriterPtr xtwpWriter = NULL;

	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	int iIndex;


	vector<State>::iterator				itState;
	vector<Transition>::iterator		itTransition;


	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//################# printFsmXML ##################
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //1.0 버전으로 생성

	
	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"APRE");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"fsm");

	xmlDocSetRootElement(xdpDoc, xnpRoot); 

		//###################################################
		//State List 설정
		//###################################################

//		xnpCurTemp = xnpCur;
		xnpState = xmlNewChild(xnpRoot, NULL, (xmlChar*)"state_list", NULL);

		sprintf(buf, "%d", p_cpFSMManager->cStateVector.vcState.size());
		xapCur = xmlNewProp(xnpState, (xmlChar*)"number", (xmlChar*)buf);



			//###################################################
			//State 설정
			//###################################################
			
//			xnpCurTemp = xnpCur;
			for (itState = p_cpFSMManager->cStateVector.vcState.begin(); itState != p_cpFSMManager->cStateVector.vcState.end(); ++itState)
			{
				xnpCur = xmlNewChild(xnpState, NULL, (xmlChar*)"state", NULL);

				sprintf(buf, "%u", itState->uiStateID);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);

				if (itState->uiStateID == 0)
					sprintf(buf, "initial state");
				else if (itState->uiStateID == 1)
					sprintf(buf, "final state");
				else
					sprintf(buf, "message format id %u", itState->cMessageFormat.uiMessageFormatIDArrange);

				xapCur = xmlNewProp(xnpCur, (xmlChar*)"info", (xmlChar*)buf);

				if (itState->uiStateID != 0 && itState->uiStateID != 1)
				{
					sprintf(buf, "%5.2lf%%", itState->dCoverage*100 );
					xapCur = xmlNewProp(xnpCur, (xmlChar*)"coverage", (xmlChar*)buf);
				}
				else
				{
					sprintf(buf, "%u", itState->uiCount);
					xapCur = xmlNewProp(xnpCur, (xmlChar*)"count", (xmlChar*)buf);
				}
			}



		//###################################################
		//Transition List 설정
		//###################################################

//		xnpCurTemp = xnpCur;
		xnpTransition = xmlNewChild(xnpRoot, NULL, (xmlChar*)"transition_list", NULL);

		sprintf(buf, "%d", p_cpFSMManager->cvTransitionVector.size());
		xapCur = xmlNewProp(xnpTransition, (xmlChar*)"number", (xmlChar*)buf);



			//###################################################
			//Transition 설정
			//###################################################
			
//			xnpCurTemp = xnpCur;
			for (itTransition = p_cpFSMManager->cvTransitionVector.begin(); itTransition != p_cpFSMManager->cvTransitionVector.end(); ++itTransition)
			{
				xnpCur = xmlNewChild(xnpTransition, NULL, (xmlChar*)"transition", NULL);

				sprintf(buf, "%u", itTransition->uiTransitionID);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);

				sprintf(buf, "%u", itTransition->uiCount);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"count", (xmlChar*)buf);

				sprintf(buf, "%.1lf%%", itTransition->dProbability*100);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"transition_probability", (xmlChar*)buf);


				//###################################################
				//Start State 설정
				//###################################################

				xnpState = xmlNewChild(xnpCur, NULL, (xmlChar*)"startstate", NULL);

					xnpCur2 = xmlNewChild(xnpState, NULL, (xmlChar*)"state", NULL);
					
					sprintf(buf, "%u", itTransition->cStartState.uiStateID);
					xapCur = xmlNewProp(xnpCur2, (xmlChar*)"id", (xmlChar*)buf);

					if (itTransition->cStartState.uiStateID == 0)
						sprintf(buf, "initial state");
					else if (itTransition->cStartState.uiStateID == 1)
						sprintf(buf, "final state");
					else
						sprintf(buf, "message format id %u", itTransition->cStartState.cMessageFormat.uiMessageFormatIDArrange);

					xapCur = xmlNewProp(xnpCur2, (xmlChar*)"info", (xmlChar*)buf);


					if (itTransition->cStartState.uiStateID != 0 && itTransition->cStartState.uiStateID != 1)
					{
						sprintf(buf, "%5.2lf%%", itTransition->cStartState.dCoverage*100 );
						xapCur = xmlNewProp(xnpCur2, (xmlChar*)"coverage", (xmlChar*)buf);
					}
					else
					{
						sprintf(buf, "%u", itTransition->cStartState.uiCount);
						xapCur = xmlNewProp(xnpCur2, (xmlChar*)"count", (xmlChar*)buf);
					}
				//###################################################
				//End State 설정
				//###################################################
				xnpState = xmlNewChild(xnpCur, NULL, (xmlChar*)"endstate", NULL);

					xnpCur2 = xmlNewChild(xnpState, NULL, (xmlChar*)"state", NULL);
					
					sprintf(buf, "%u", itTransition->cEndState.uiStateID);
					xapCur = xmlNewProp(xnpCur2, (xmlChar*)"id", (xmlChar*)buf);

					if (itTransition->cEndState.uiStateID == 0)
						sprintf(buf, "initial state");
					else if (itTransition->cEndState.uiStateID == 1)
						sprintf(buf, "final state");
					else
						sprintf(buf, "message format id %u", itTransition->cEndState.cMessageFormat.uiMessageFormatIDArrange);

					xapCur = xmlNewProp(xnpCur2, (xmlChar*)"info", (xmlChar*)buf);


					if (itTransition->cEndState.uiStateID != 0 && itTransition->cEndState.uiStateID != 1)
					{
						sprintf(buf, "%5.2lf%%", itTransition->cEndState.dCoverage*100 );
						xapCur = xmlNewProp(xnpCur2, (xmlChar*)"coverage", (xmlChar*)buf);
					}
					else
					{
						sprintf(buf, "%u", itTransition->cEndState.uiCount);
						xapCur = xmlNewProp(xnpCur2, (xmlChar*)"count", (xmlChar*)buf);
					}
			}

	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "EUC-KR", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printFsmXML() : end");
	cTimeChecker.print();
	
	return;
}
void ExtractMethodandHeaderName(SequenceVector* p_cpMsgSequenceVector, char* caSaveFileName, char* caResultDirectory)
{
	puts("ExtractMethodandHeaderName() : start");

	TimeChecker			cTimeChecker;
	
	cTimeChecker.startClock();
	//puts(caResultDirectory);
	p_cpMsgSequenceVector->ExtractMethodandHeaderName(caSaveFileName, caResultDirectory);

	cTimeChecker.endClock();

	puts("ExtractMethodandHeaderName() : end");
	cTimeChecker.print();

	return;



}
//#################################################################################################################
void printResultLog(char* p_caResultTrafficDirectory, char* p_cpFileName, FlowHash* p_cpFlowHash, SequenceVector* p_cpFlowSequenceVector, SequenceVector* p_cpMessageSequenceVector, SequenceVector* p_cpFieldSequenceVector, FieldFormatList* p_cpFieldFormatList, MessageFormatList* p_cpMessageFormatList, FlowFormatList* p_cpFlowFormatList)
{
	FILE *fp;

	vector<State>::iterator				itState;
	double	dTotalCoverage;
	u_int32_t uiTotalCoverMsgCount;

	list<FieldFormat>::iterator			itFieldFormat;
	int iRequestSFV, iResponseSFV, iRequestDFV, iResponseDFV, SFV;

	list<MessageFormat>::iterator		itMessageFormat;
	int iRequestFormat, iResponseFormat, iNum_MessageFormat;
	list<FlowFormat>::iterator			itFlowFormat;


	//###################################################
	//File 출력
	//###################################################

	if ( (fp = fopen(p_cpFileName, "wt")) == NULL )
		g_err((char*)"printResultLog:result log file fopen error");
	else
	{
		fprintf(fp,"                               Configuration                               \r\n");
		
		fprintf(fp, "Traffic Directory : %s\r\n", p_caResultTrafficDirectory);
		fprintf(fp, "Minimum support value of Session set : %d\r\n", g_iMinSuppUnitFlow);
		fprintf(fp, "Minimum support value of Site-Specific Session set : %d\r\n", g_iMinSuppUnitServer);
		fprintf(fp, "Minimum length of field format(byte) : %d\r\n", g_iMinContentLength);
		fprintf(fp, "Maximum length of message to be analyzed : %d\r\n", g_iMaxSequenceLength);
		fprintf(fp, "Maximum postion variance used in Variance Analysis : %lf\r\n", g_iMaxSuppUnitPositionVariance);
		fprintf(fp, "Range to analyze : %d\r\n", g_irange);
		fprintf(fp,"\r\n");
		fprintf(fp,"\r\n");
		fprintf(fp,"\r\n");

		fprintf(fp,"                                  Summary                                  \r\n");

		fprintf(fp,"##### Traffic Info. #####\r\n");
		fprintf(fp,"flow : %7llu pkt : %7llu byte : %7llu\r\n",g_cTotalVolume.flow,g_cTotalVolume.pkt,g_cTotalVolume.byte);
		fprintf(fp,"message : %7d(Request : %d, Response : %d)\r\n",p_cpMessageSequenceVector->cvSequenceVector.size(),g_iDIREC_REQUESTMsgCount,g_iDIREC_RESPONSEMsgCount);
		fprintf(fp,"\r\n");

		fprintf(fp,"##### Format Info. #####\r\n");
		fprintf(fp,"Field Format : %3d\r\n", p_cpFieldFormatList->clFieldFormat.size());

		SFV=iRequestSFV = iResponseSFV = iRequestDFV = iResponseDFV = 0;
		for(itFieldFormat = p_cpFieldFormatList->clFieldFormat.begin() ; itFieldFormat != p_cpFieldFormatList->clFieldFormat.end() ; ++itFieldFormat)
		{
			if(itFieldFormat->cvAdditionalContentVector.size() == 0)
			{
			
				SFV++;
			}
			else
			{
				
				g_err((char*)"printResultLog:itFieldFormat->cvAdditionalContentVector.size() != 0");
			}
		}

	
		fprintf(fp,"      SF(v) : %3d \r\n", SFV);

		fprintf(fp,"Message Format : %3d\r\n", p_cpMessageFormatList->clMessageFormat.size());

	
		iNum_MessageFormat=p_cpMessageFormatList->clMessageFormat.size();
	

		fprintf(fp,"      Num of Message Format : %3d\r\n",iNum_MessageFormat);

		dTotalCoverage = 0;
		uiTotalCoverMsgCount = 0;

		fprintf(fp,"      Coverage : msg = %.2lf%%(%d/%d)\r\n",(double)(p_cpMessageFormatList->uiTotalCoverdMessage * 100 ) / p_cpMessageSequenceVector->cvSequenceVector.size(), 
			p_cpMessageFormatList->uiTotalCoverdMessage, p_cpMessageSequenceVector->cvSequenceVector.size());

		fprintf(fp,"Flow Format : %3d\r\n", p_cpFlowFormatList->clFlowFormat.size());

		fprintf(fp,"      Coverage : flow = %.2f%%(%llu/%llu) msg = %.2f%%(%llu/%d) pkt = %.2f%%(%llu/%llu) byte = %.2f%%(%llu/%llu)\r\n",
			(float)g_cFlowVolume.flow*100/g_cTotalVolume.flow, g_cFlowVolume.flow, g_cTotalVolume.flow,
			(float)g_cFlowVolume.msg*100/p_cpMessageSequenceVector->cvSequenceVector.size(), g_cFlowVolume.msg, p_cpMessageSequenceVector->cvSequenceVector.size(),
			(float)g_cFlowVolume.pkt*100/g_cTotalVolume.pkt, g_cFlowVolume.pkt, g_cTotalVolume.pkt,
			(float)g_cFlowVolume.byte*100/g_cTotalVolume.byte, g_cFlowVolume.byte, g_cTotalVolume.byte);
		fprintf(fp,"\r\n");

		fclose(fp);
	}

	//###################################################
	//Console 출력
	//###################################################

	setBackgroundColorBrightBlue();
	printf("                               Configuration                               \x1b[0;0m\r\n");
	setDefault();
	printf("Traffic Directory : %s\r\n", p_caResultTrafficDirectory);
	printf("Minimum support value of Session set : %d\r\n", g_iMinSuppUnitFlow);
	printf("Minimum support value of Site-Specific Session set : %d\r\n", g_iMinSuppUnitServer);
	printf("Minimum length of field format(byte) : %d\r\n", g_iMinContentLength);
	printf("Maximum length of message to be analyzed : %d\r\n", g_iMaxSequenceLength);
	printf("Maximum postion variance used in Variance Analysis : %lf\r\n", g_iMaxSuppUnitPositionVariance);
	printf("Range to analyze : %d\r\n", g_irange);

	setBackgroundColorBrightBlue();
	printf("                                  Summary                                  \x1b[0;0m\r\n");
	setDefault();

	setTextColorBlue();

	printf("##### Traffic Info. #####\r\n");

	setDefault();
	printf("flow : %7llu pkt : %7llu byte : %7llu\r\n",g_cTotalVolume.flow,g_cTotalVolume.pkt,g_cTotalVolume.byte);
	printf("message : %7d(Request : %d, Response : %d)\r\n",p_cpMessageSequenceVector->cvSequenceVector.size(),g_iDIREC_REQUESTMsgCount,g_iDIREC_RESPONSEMsgCount);
	printf("\r\n");

	setTextColorBlue();

	printf("##### Format Info. #####\r\n");

	setDefault();

	setTextColorGreen();

	printf("Field Format : %3d\r\n", p_cpFieldFormatList->clFieldFormat.size());

	setDefault();

	iRequestSFV = iResponseSFV = iRequestDFV = iResponseDFV = 0;
	SFV=p_cpFieldFormatList->clFieldFormat.size();
	
	printf("      SF(v) : %3d\r\n", SFV);
	
	setTextColorGreen();

	printf("Message Format : %3d\r\n", p_cpMessageFormatList->clMessageFormat.size());

	setDefault();

	iRequestFormat = iResponseFormat = 0;
	iNum_MessageFormat=p_cpMessageFormatList->clMessageFormat.size();


	printf("      Num of Message Format : %3d\r\n",iNum_MessageFormat);

	setTextColorRed();

	dTotalCoverage = 0;
	uiTotalCoverMsgCount = 0;
	//printf("%d	%d",p_cpMessageFormatList->uiTotalCoverdMessage, p_cpMessageSequenceVector->cvSequenceVector.size());
	printf("      Coverage : msg = %.2lf%%(%d/%d)\r\n",(double)(p_cpMessageFormatList->uiTotalCoverdMessage * 100 ) / p_cpMessageSequenceVector->cvSequenceVector.size(), 
			p_cpMessageFormatList->uiTotalCoverdMessage, p_cpMessageSequenceVector->cvSequenceVector.size());
	setDefault();

	setTextColorGreen();

	printf("Flow Format : %3d\r\n", p_cpFlowFormatList->clFlowFormat.size());

	setDefault();

	setTextColorRed();
	
	printf("      Coverage : flow = %.2f%%(%llu/%llu) msg = %.2f%%(%llu/%d) pkt = %.2f%%(%llu/%llu) byte = %.2f%%(%llu/%llu)\r\n",
		(float)g_cFlowVolume.flow*100/g_cTotalVolume.flow, g_cFlowVolume.flow, g_cTotalVolume.flow,
		(float)g_cFlowVolume.msg*100/p_cpMessageSequenceVector->cvSequenceVector.size(), g_cFlowVolume.msg, p_cpMessageSequenceVector->cvSequenceVector.size(),
		(float)g_cFlowVolume.pkt*100/g_cTotalVolume.pkt, g_cFlowVolume.pkt, g_cTotalVolume.pkt,
		(float)g_cFlowVolume.byte*100/g_cTotalVolume.byte, g_cFlowVolume.byte, g_cTotalVolume.byte);
	printf("\r\n");

	setDefault();

	setTextColorBlue();
/*
	printf("##### FSM Info. #####\r\n");

	setDefault();

	setTextColorGreen();

	printf("state : %3d transition : %3d\r\n",p_cpFSMManager->cStateVector.vcState.size(),p_cpFSMManager->cvTransitionVector.size());

	setDefault();

	setTextColorRed();

	printf("      Coverage : flow = %.2f%%(%llu/%llu) msg = %.2lf%%(%d/%d)\r\n",
		(float)g_cTotalVolume.flow*100/g_cTotalVolume.flow, g_cTotalVolume.flow, g_cTotalVolume.flow,
		(double)(uiTotalCoverMsgCount*100)/p_cpMessageSequenceVector->cvSequenceVector.size(), uiTotalCoverMsgCount, p_cpMessageSequenceVector->cvSequenceVector.size());
		*/

	setDefault();


	return;
}
//#######################################################################################################################
void printTraceInfoXML(char* p_cpFileName, FlowHash* p_cpFlowHash, char* p_caTargetDirectory, MessageFormatList* p_cpMessageFormatList)
{
	puts("printTraceInfoXML() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpFileList = NULL;
	xmlNodePtr xnpFlowList = NULL;
	xmlNodePtr xnpFlow = NULL;
	xmlNodePtr xnpMessage = NULL;
	xmlNodePtr xnpPacket = NULL;

	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	char bufValue[100] = {0,};

	int				iTotalFwpFileCount;
	struct dirent	**spDirEntry;
	int				iFileCount = 0;
	char			caTargetFwpFileName[1024]={0,};
	DIR				*dp;
	int				iIndex;
	struct			stat statbuf;

	vector<FlowEntry>::iterator	itFlow;
	MessageContainer			*MsgGo;
	PacketContainer				*pktGo;

	struct in_addr addr;
	char srcip[20], dstip[20];
	char srcMAC_temp[18], dstMAC_temp[18];

	int iCurMessage = 0;
	int	iCurPacket = 0;

	list<MessageFormat>::iterator	itMessageFormat;
	vector<u_int32_t>::iterator		itCoveredMessagdId;
	bool bFlag = false;
	int iDataLen = 0;


	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//################ printTraceInfoXML ################
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //xml 1.0 버전으로 생성

	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"AutoReEngine");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"trace_info");
	sprintf(buf, "%d", g_iTotalFwpFileCount);
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"number-Of-trace", (xmlChar*)buf);

	xmlDocSetRootElement(xdpDoc, xnpRoot);


		//###################################################
		//File List 설정
		//###################################################

		xnpFileList = xmlNewChild(xnpRoot, NULL, (xmlChar*)"file_list", NULL);
		sprintf(buf, "%d", g_iTotalFwpFileCount);
		xapCur = xmlNewProp(xnpFileList, (xmlChar*)"number", (xmlChar*)buf);

			//###################################################
			//File 설정
			//###################################################

			iTotalFwpFileCount = scandir(p_caTargetDirectory, &spDirEntry, isFwpFile, dicSort);		//count fwp file

			if ( (dp = opendir(p_caTargetDirectory)) != NULL )
			{
				chdir(p_caTargetDirectory);
				for (iIndex=0; iIndex < iTotalFwpFileCount; iIndex++ )
				{
					lstat(spDirEntry[iIndex]->d_name, &statbuf);
					if ( S_ISREG(statbuf.st_mode) )
					{
						sprintf(caTargetFwpFileName, "%s/%s", p_caTargetDirectory, spDirEntry[iIndex]->d_name);

						xnpCur = xmlNewChild(xnpFileList, NULL, (xmlChar*)"file", NULL);
						sprintf(buf, "%d", iFileCount++);
						xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);
						//file 이름 pcap으로 저장
						g_changeExtention(buf, spDirEntry[iIndex]->d_name, (char*)"pcap");
						xapCur = xmlNewProp(xnpCur, (xmlChar*)"name", (xmlChar*)buf);
					}
				}
				closedir(dp);
			}
			else
			{
				printf("dir : %s\n", p_caTargetDirectory);
				g_err((char*)"FlowHash::loadFlow() : can't open dir");
			}

		//###################################################
		//Flow List 설정
		//###################################################
		xnpFlowList = xmlNewChild(xnpRoot, NULL, (xmlChar*)"flow_list", NULL);
		sprintf(buf, "%d", g_iFlowCount);
		xapCur = xmlNewProp(xnpFlowList, (xmlChar*)"number", (xmlChar*)buf);

			//###################################################
			//Flow 설정
			//###################################################

			//모든 flow 순회
			for (itFlow = p_cpFlowHash->m_cFlowTwoWayContainerList.begin(); itFlow != p_cpFlowHash->m_cFlowTwoWayContainerList.end(); ++itFlow)
			{
				xnpFlow = xmlNewChild(xnpFlowList, NULL, (xmlChar*)"flow", NULL);
				sprintf(buf, "%d", itFlow->m_cpFlow->m_iFlowID);
				xapCur = xmlNewProp(xnpFlow, (xmlChar*)"id", (xmlChar*)buf);
				sprintf(buf, "%d", itFlow->m_cpFlow->m_iFileID);
				xapCur = xmlNewProp(xnpFlow, (xmlChar*)"from_file_id", (xmlChar*)buf);

/*
				if (itFlow->m_cpFlow->flow.pad & WITH_MAC)
				{
					g_convertMACToString(srcMAC_temp, itFlow->m_cpFlow->flow.srcMAC);
					g_convertMACToString(dstMAC_temp, itFlow->m_cpFlow->flow.dstMAC);
					sprintf(buf, "%-18s -- %-18s", srcMAC_temp, dstMAC_temp);
				}
				else
				{
					*/
					memset(&addr, 0, sizeof(struct in_addr) );
					memcpy(&addr, &itFlow->m_cpFlow->flow.srcaddr, 4);
					strcpy(srcip, (const char *)inet_ntoa(addr) );
					memcpy(&addr, &itFlow->m_cpFlow->flow.dstaddr, 4);
					strcpy(dstip, (const char *)inet_ntoa(addr) );
					sprintf(buf, "%-15s : %5d -- %2d -- %-15s : %5d", srcip, itFlow->m_cpFlow->flow.srcport, itFlow->m_cpFlow->flow.prot, dstip, itFlow->m_cpFlow->flow.dstport);
			//	}
				xapCur = xmlNewProp(xnpFlow, (xmlChar*)"info", (xmlChar*)buf);

				sprintf(buf, "%d", itFlow->m_cpFlow->forward.dMsgs + itFlow->m_cpFlow->backward.dMsgs);
				xapCur = xmlNewProp(xnpFlow, (xmlChar*)"msgs", (xmlChar*)buf);
				sprintf(buf, "%d", itFlow->m_cpFlow->forward.dPkts + itFlow->m_cpFlow->backward.dPkts);
				xapCur = xmlNewProp(xnpFlow, (xmlChar*)"pkts", (xmlChar*)buf);
				sprintf(buf, "%d", itFlow->m_cpFlow->forward.dOctets + itFlow->m_cpFlow->backward.dOctets);
				xapCur = xmlNewProp(xnpFlow, (xmlChar*)"bytes", (xmlChar*)buf);

				//###################################################
				//Message 설정
				//###################################################
				for ( MsgGo = itFlow->m_cpFlow->headMsg;  MsgGo!=NULL;  MsgGo = MsgGo->next) //message
				{
					xnpMessage = xmlNewChild(xnpFlow, NULL, (xmlChar*)"message", NULL);
					sprintf(buf, "%d", iCurMessage++);
					xapCur = xmlNewProp(xnpMessage, (xmlChar*)"id", (xmlChar*)buf);

					if(itFlow->m_cpFlow->isC2SDirection(&MsgGo->msg)) //forward message
						sprintf(buf, "request");
					else
						sprintf(buf, "response");
					xapCur = xmlNewProp(xnpMessage, (xmlChar*)"direction", (xmlChar*)buf);

/*
					if (MsgGo->msg.flag & HAVE_MAC)
					{
						g_convertMACToString(srcMAC_temp, MsgGo->msg.ether_src);
						g_convertMACToString(dstMAC_temp, MsgGo->msg.ether_dst);
						sprintf(buf, "%-18s to %-18s", srcMAC_temp, dstMAC_temp);
					}
					else
					{*/
						memset(&addr, 0, sizeof(struct in_addr) );
						memcpy(&addr, &MsgGo->msg.src_addr, 4);
						strcpy(srcip, (const char *)inet_ntoa(addr) );
						memcpy(&addr, &MsgGo->msg.dst_addr, 4);
						strcpy(dstip, (const char *)inet_ntoa(addr) );
						sprintf(buf, "%-15s : %5d -- %2d to %-15s : %5d", srcip, MsgGo->msg.src_port, MsgGo->msg.ip_proto, dstip, MsgGo->msg.dst_port);
				//	}
					xapCur = xmlNewProp(xnpMessage, (xmlChar*)"header", (xmlChar*)buf);
					sprintf(buf, "%d", MsgGo->msg.pkts);
					xapCur = xmlNewProp(xnpMessage, (xmlChar*)"pkts", (xmlChar*)buf);
					sprintf(buf, "%d", MsgGo->msg.real_payload_len);
					xapCur = xmlNewProp(xnpMessage, (xmlChar*)"msg_len", (xmlChar*)buf);

					//MatchedMessageFormat ID 설정

					bFlag=false;
					for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
					{
						for (itCoveredMessagdId = itMessageFormat->uivCoverdMessageIdList.begin(); itCoveredMessagdId != itMessageFormat->uivCoverdMessageIdList.end(); ++itCoveredMessagdId)
						{
							if (iCurMessage-1 == *itCoveredMessagdId)
							{
								bFlag=true;
								break;
							}
						}
						if (bFlag) break;
					}
					if (bFlag)
					{
						sprintf(buf, "%u", itMessageFormat->uiMessageFormatIDArrange);
						xapCur = xmlNewProp(xnpMessage, (xmlChar*)"matched_message_format_id", (xmlChar*)buf);
					}

						//###################################################
						//Packet 설정
						//###################################################
						for(pktGo = MsgGo->headPkt ; pktGo!=NULL ; pktGo=pktGo->next)
						{
							xnpPacket = xmlNewChild(xnpMessage, NULL, (xmlChar*)"packet", NULL);
							pktGo->m_uPacketID = iCurPacket++;
							sprintf(buf, "%d", pktGo->m_uPacketID);
							xapCur = xmlNewProp(xnpPacket, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%d", pktGo->pkt.real_pkt_len);
							xapCur = xmlNewProp(xnpPacket, (xmlChar*)"pkt_len", (xmlChar*)buf);
							iDataLen = (int)(pktGo->pkt.stored_pkt_len - pktGo->pkt.stored_payload_len + pktGo->pkt.real_payload_len) < 65536 ? (int)(pktGo->pkt.real_payload_len) : (pktGo->pkt.stored_payload_len);
							sprintf(buf, "%d", iDataLen);
							xapCur = xmlNewProp(xnpPacket, (xmlChar*)"data_len", (xmlChar*)buf);

							//###################################################
							//Packet의 Data 설정
							//###################################################
							pktGo->extractPayload(buf);
							xnpCur = xmlNewChild(xnpPacket, NULL, (xmlChar*)"data", (xmlChar*)buf);
						}
				}
			}

	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "utf-8", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printTraceInfoXML() : end");
	cTimeChecker.print();

	return;

}
//########################################################################################
void setCoverageMessageFormat(SequenceVector* p_cpFlowSequenceVector, SequenceVector* p_cpMessageSequenceVector, MessageFormatList* p_cpMessageFormatList, FlowHash* p_cpFlowHash)
{
	vector<Sequence>::iterator		itSequence;
	list<FormatId>::iterator		itMessageID;
	list<MessageFormat>::iterator	itMessageFormat;

	int			iIndex;

	vector<FlowEntry>::iterator		itFlow;
	MessageContainer		*MsgGo;


	//FlowSequence 순회
	for (itSequence = p_cpFlowSequenceVector->cvSequenceVector.begin(); itSequence != p_cpFlowSequenceVector->cvSequenceVector.end(); ++itSequence)
	{
		itMessageID = itSequence->clFormatIdListFinal.begin();

		//Message Format ID 순회
		while (itMessageID != itSequence->clFormatIdListFinal.end())	//IdListFinal은 OIM, FII 정렬되어 있음
		{
			for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
			{
				if (itMessageID->uiFormatID == itMessageFormat->uiMessageFormatID)
				{
					itMessageFormat->uiCoverdMessage++;
					p_cpMessageFormatList->uiTotalCoverdMessage++;

					//FlowHash에서 MessageID 찾아 CoveredMessage에 기록
					for (itFlow = p_cpFlowHash->m_cFlowTwoWayContainerList.begin(); itFlow != p_cpFlowHash->m_cFlowTwoWayContainerList.end(); ++itFlow)
					{
						if (itSequence->uiSequenceID == itFlow->m_cpFlow->m_iFlowID)
						{
							for ( MsgGo = itFlow->m_cpFlow->headMsg;  MsgGo->m_uMessageIDinFlow != itMessageID->uiFormatIndexInFlow;  MsgGo = MsgGo->next); //message
							if (MsgGo == NULL)
							{
								g_err((char*)"setCoverageMessageFormat():Finding of coveredMessage about the MessageFormat is error");
							}
							itMessageFormat->uivCoverdMessageIdList.push_back(MsgGo->m_uMessageID);
						}
					}
				}
			}

			itMessageID++;
		}
	}

	for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
	{
		itMessageFormat->dCoverage = (double)itMessageFormat->uiCoverdMessage / p_cpMessageSequenceVector->cvSequenceVector.size();
		std::sort(itMessageFormat->uivCoverdMessageIdList.begin(), itMessageFormat->uivCoverdMessageIdList.end());
	}

	p_cpMessageFormatList->dTotalCoverage = (double)p_cpMessageFormatList->uiTotalCoverdMessage / p_cpMessageSequenceVector->cvSequenceVector.size();

	return;
}
//#######################################################################################################################################
void printMessageFormatListWithPacketXML(char* p_cpFileName, MessageFormatList* p_cpMessageFormatList, FlowHash* p_cpFlowHash)
{
	puts("printMessageFormatListWithPacketXML() : start");

	TimeChecker				cTimeChecker;

	xmlDocPtr xdpDoc = NULL;
	xmlNodePtr xnpCur = NULL;
	xmlAttrPtr xapCur = NULL;

	xmlNodePtr xnpRoot = NULL;
	xmlNodePtr xnpFieldFormat = NULL;
	xmlNodePtr xnpMessageFormat = NULL;
	xmlNodePtr xnpMessage = NULL;
	xmlNodePtr xnpPacket = NULL;
	xmlNodePtr xnpData = NULL;


	xmlChar* xcStr;

	char buf[65536*4] = {0,};
	int iIndex, iIndex2;

	char bufValue[100] = {0,};
	int iIndexValue;

	list<FieldFormat>::iterator itFieldFormat;
	list<FieldFormat>::iterator itFieldFormat2;
	list<MessageFormat>::iterator itMessageFormat;
	vector<Content>::iterator	itContent;
	list<FieldFormat>::iterator itSplitFieldFormat,it_next;

	int iNumOfFieldFormatConverted, iNumOfSplitFieldFormat;

	vector<MessageEntry>::iterator	itMessage;
	PacketContainer				*pktGo;

	struct in_addr addr;
	char srcip[20], dstip[20];
	char srcMAC_temp[18], dstMAC_temp[18];

	vector<u_int32_t>::iterator		itCoveredMsgID;
	int messageCount;


	cTimeChecker.startClock();

	//###################################################
	//###################################################
	//###################################################
	//######### printMessageFormatListXML ###############
	//###################################################
	//###################################################
	//###################################################

	xdpDoc = xmlNewDoc((xmlChar*)"1.0"); //xml 1.0 버전으로 생성

	//###################################################
	//rootNode 지정
	//###################################################

	xnpRoot = xmlNewNode(NULL, (xmlChar*)"AutoReEngine");

	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"version", (xmlChar*)"1.0.0");
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"info", (xmlChar*)"messageformat_list");
	sprintf(buf, "%d", g_iDIREC_REQUESTMsgCount+g_iDIREC_RESPONSEMsgCount);
	xapCur = xmlNewProp(xnpRoot, (xmlChar*)"NumOfMessage", (xmlChar*)buf);


	xmlDocSetRootElement(xdpDoc, xnpRoot);

		//###################################################
		//Message Format List 설정
		//###################################################
		xnpMessageFormat = xmlNewChild(xnpRoot, NULL, (xmlChar*)"messageformat_list", NULL);
		sprintf(buf, "%d", p_cpMessageFormatList->clMessageFormat.size());
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"number", (xmlChar*)buf);
//		sprintf(buf, "%.2lf%%", p_cpMessageFormatList->dCompression*100);
//		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"compression", (xmlChar*)buf);
		sprintf(buf, "%.2lf%%", p_cpMessageFormatList->dTotalCoverage*100);
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"coverage", (xmlChar*)buf);
		sprintf(buf, "%u", p_cpMessageFormatList->uiTotalCoverdMessage);
		xapCur = xmlNewProp(xnpMessageFormat, (xmlChar*)"NumOfCoveredMessage", (xmlChar*)buf);

			//###################################################
			//Message Format 설정
			//###################################################

//			xnpCurTemp = xnpCur;
			for (itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin(); itMessageFormat != p_cpMessageFormatList->clMessageFormat.end(); ++itMessageFormat)
			{
				xnpCur = xmlNewChild(xnpMessageFormat, NULL, (xmlChar*)"messageformat", NULL);

				sprintf(buf, "%u", itMessageFormat->uiMessageFormatIDArrange);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"id", (xmlChar*)buf);
				sprintf(buf, "%s%s", itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_REQUEST ? "request" : ""
									,itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"direction", (xmlChar*)buf);

				itMessageFormat->cMessageHeader.extractCharP(buf);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"header", (xmlChar*)buf);

				iNumOfSplitFieldFormat=0;
				iNumOfFieldFormatConverted=0;
				
				sprintf(buf, "%d", itMessageFormat->clFieldFormat.size() + itMessageFormat->clDynamicFieldFormat.size() - iNumOfFieldFormatConverted + iNumOfSplitFieldFormat);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"number-of-fieldformat", (xmlChar*)buf);
				sprintf(buf, "%.2lf%%", itMessageFormat->dCoverage*100);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"coverage", (xmlChar*)buf);
				sprintf(buf, "%u", itMessageFormat->uiCoverdMessage);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"NumOfCoveredMessage", (xmlChar*)buf);

				sprintf(buf,"%.3lf",itMessageFormat->dSupp_unit_flow);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"session_support", (xmlChar*)buf);

				sprintf(buf,"%.3lf",itMessageFormat->dSupp_unit_server);
				xapCur = xmlNewProp(xnpCur, (xmlChar*)"sessionset_support", (xmlChar*)buf);
//				sprintf(buf, "%u", itMessageFormat->uivIdentifiedMessageIdList.size());
//				xapCur = xmlNewProp(xnpCur, (xmlChar*)"NumOfIdentifiedMessage", (xmlChar*)buf);

				//Message Format의 자식 Field Format 설정
				itFieldFormat = itMessageFormat->clFieldFormat.begin();
				itFieldFormat2 = itMessageFormat->clDynamicFieldFormat.begin();
				iIndex=0;

				while ((itFieldFormat != itMessageFormat->clFieldFormat.end()) && (itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
				{
					if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat > iIndex)
					{
						
						
							xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
							sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
													,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
													,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
													,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
							sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
												,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
							xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);

								//Field Format의 자식 element 설정
								itFieldFormat->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

								if(itFieldFormat->cvAdditionalContentVector.size())
								{
									for(itContent = itFieldFormat->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
									{
										itContent->extractCharP(buf);
										//sprintf(bufValue, "%s%d", "value",iIndexValue);
										//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
										xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
									}
								}


								sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

								
								
								sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);
								


						
						

						iIndex++;
						itFieldFormat++;
					}
					else if (itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat == iIndex)
					{
						xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
						sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
																	,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);

						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
												,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
												,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
												,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
						sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);

							//Field Format의 자식 element 설정

							if (!(itFieldFormat2->uiType & GAP_TYPE))
							{
								
								
								itFieldFormat2->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								
								sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

								sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
							}


							

						itFieldFormat2++;
					}
					else
						g_err((char*)"printFormatListXML() : In messageformat node, field format parsing error");
				}


				if ((itFieldFormat2 != itMessageFormat->clDynamicFieldFormat.end()))
				{
					xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
					sprintf(buf, "%u:(%d,%d)", itMessageFormat->uiMessageFormatIDArrange, itFieldFormat2->cDynamicFieldPosition.iPreFieldFormatIndexInMessageFormat
																,itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat != itMessageFormat->clFieldFormat.size() ? itFieldFormat2->cDynamicFieldPosition.iPostFieldFormatIndexInMessageFormat : -2);
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
					sprintf(buf, "%s%s%s%s", itFieldFormat2->uiType & SFV_TYPE ? "SF(v)" : ""
											,itFieldFormat2->uiType & DFV_TYPE ? "DF(v)" : ""
											,itFieldFormat2->uiType & DF_TYPE ? "DF" : ""
											,itFieldFormat2->uiType & GAP_TYPE ? "GAP" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
					sprintf(buf, "%s%s", itFieldFormat2->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
										,itFieldFormat2->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
					xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);

						//Field Format의 자식 element 설정

						if (!(itFieldFormat2->uiType & GAP_TYPE))
						{
							if(itFieldFormat2->uiType & DFV_TYPE)
							{
								itFieldFormat2->cContent.extractCharP(buf);
								if( itFieldFormat2->cContent.vcChars.size()==0)
									strcpy(buf,"&amp;BLANK&amp;");
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

								for(itContent = itFieldFormat2->cvAdditionalContentVector.begin(), iIndexValue=2; itContent != itFieldFormat2->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
								{
									itContent->extractCharP(buf);
									//sprintf(bufValue, "%s%d", "value",iIndexValue);
									//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
									if( itContent->vcChars.size()==0)
										strcpy(buf,"&amp;BLANK&amp;");
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								}
							}
							else if(itFieldFormat2->uiType & SFV_TYPE)
							{
								itFieldFormat2->cContent.extractCharP(buf);
								xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
							}


							sprintf(buf, "%u", itFieldFormat2->cContent.uiOffset);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cContent.uiDepth);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMinLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiMaxLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat2->cDynamicFieldPosition.uiAvgLength);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"avg_length", (xmlChar*)buf);
						}


						if (itFieldFormat2->uiSemanticsCode)
						{
							sprintf(buf, "%s%s%s%s%s%s", itFieldFormat2->uiSemanticsCode & SE_MSG_TYPE ? "MSG_TYPE" : ""
													,itFieldFormat2->uiSemanticsCode & SE_MSG_LEN ? "MSG_LEN" : ""
													,itFieldFormat2->uiSemanticsCode & SE_HOST_ID ? "HOST_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_SESSION_ID ? "SESSION_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_TRANS_ID ? "TRANS_ID" : ""
													,itFieldFormat2->uiSemanticsCode & SE_ACCUMULATORS ? "ACCUMULATORS" : "");
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"semantics_code", (xmlChar*)buf);
						}
				}
				if ((itFieldFormat != itMessageFormat->clFieldFormat.end()))
				{
					
				
						xnpFieldFormat = xmlNewChild(xnpCur, NULL, (xmlChar*)"fieldformat", NULL);
						sprintf(buf, "%u", itFieldFormat->uiFieldFormatIDArrange);
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"id", (xmlChar*)buf);
						sprintf(buf, "%s%s%s%s", itFieldFormat->uiType & SFV_TYPE ? "SF(v)" : ""
												,itFieldFormat->uiType & DFV_TYPE ? "DF(v)" : ""
												,itFieldFormat->uiType & DF_TYPE ? "DF" : ""
												,itFieldFormat->uiType & GAP_TYPE ? "GAP" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"type", (xmlChar*)buf);
						sprintf(buf, "%s%s", itFieldFormat->cContent.uiDirection & DIREC_REQUEST ? "request" : ""
											,itFieldFormat->cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
						xapCur = xmlNewProp(xnpFieldFormat, (xmlChar*)"direction", (xmlChar*)buf);

							//Field Format의 자식 element 설정
							itFieldFormat->cContent.extractCharP(buf);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);

							if(itFieldFormat->cvAdditionalContentVector.size())
							{
								for(itContent = itFieldFormat->cvAdditionalContentVector.begin(), iIndexValue=2 ; itContent != itFieldFormat->cvAdditionalContentVector.end() ; ++itContent, ++iIndexValue)
								{
									itContent->extractCharP(buf);
									//sprintf(bufValue, "%s%d", "value",iIndexValue);
									//xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)bufValue, (xmlChar*)buf);
									xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"value", (xmlChar*)buf);
								}
							}


							sprintf(buf, "%u", itFieldFormat->cContent.uiOffset);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"offset", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat->cContent.uiDepth);
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"depth", (xmlChar*)buf);

						
							
							sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"min_length", (xmlChar*)buf);

							sprintf(buf, "%u", itFieldFormat->cContent.vcChars.size());
							xmlNewChild(xnpFieldFormat, NULL, (xmlChar*)"max_length", (xmlChar*)buf);
							


							
					
				}

				//####### 분석하는 Message 출력

				messageCount = 0;

				for (itCoveredMsgID = itMessageFormat->uivCoverdMessageIdList.begin(); itCoveredMsgID != itMessageFormat->uivCoverdMessageIdList.end(); ++itCoveredMsgID)
				{
					xnpMessage = xmlNewChild(xnpCur, NULL, (xmlChar*)"identified_message", NULL);
					for (itMessage = p_cpFlowHash->m_cMessageContainerList.begin(); itMessage != p_cpFlowHash->m_cMessageContainerList.end(); ++itMessage)
					{
						if (*itCoveredMsgID == itMessage->m_cpMessage->m_uMessageID)
						{
							sprintf(buf, "%u", itMessage->m_cpMessage->m_uMessageID);
							xapCur = xmlNewProp(xnpMessage, (xmlChar*)"id", (xmlChar*)buf);
							sprintf(buf, "%s%s", itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_REQUEST ? "request" : ""
												,itMessageFormat->clFieldFormat.front().cContent.uiDirection & DIREC_RESPONSE ? "response" : "");
							xapCur = xmlNewProp(xnpMessage, (xmlChar*)"direction", (xmlChar*)buf);

							
							
							memset(&addr, 0, sizeof(struct in_addr) );
							memcpy(&addr, &itMessage->m_cpMessage->msg.src_addr, 4);
							strcpy(srcip, (const char *)inet_ntoa(addr) );
							memcpy(&addr, &itMessage->m_cpMessage->msg.dst_addr, 4);
							strcpy(dstip, (const char *)inet_ntoa(addr) );
							sprintf(buf, "%-15s : %5d -- %2d to %-15s : %5d", srcip, itMessage->m_cpMessage->msg.src_port, itMessage->m_cpMessage->msg.ip_proto, dstip, itMessage->m_cpMessage->msg.dst_port);
							
							xapCur = xmlNewProp(xnpMessage, (xmlChar*)"header", (xmlChar*)buf);
							sprintf(buf, "%d", itMessage->m_cpMessage->msg.pkts);
							xapCur = xmlNewProp(xnpMessage, (xmlChar*)"pkts", (xmlChar*)buf);
							sprintf(buf, "%d", itMessage->m_cpMessage->msg.real_payload_len);
							xapCur = xmlNewProp(xnpMessage, (xmlChar*)"msg_len", (xmlChar*)buf);
							//###################################################
							//Packet 설정
							//###################################################
							for(pktGo = itMessage->m_cpMessage->headPkt ; pktGo!=NULL ; pktGo=pktGo->next)
							{
								xnpPacket = xmlNewChild(xnpMessage, NULL, (xmlChar*)"packet", NULL);
								sprintf(buf, "%d", pktGo->m_uPacketID);
								xapCur = xmlNewProp(xnpPacket, (xmlChar*)"id", (xmlChar*)buf);
								sprintf(buf, "%d", pktGo->pkt.real_pkt_len);
								xapCur = xmlNewProp(xnpPacket, (xmlChar*)"pkt_len", (xmlChar*)buf);
								sprintf(buf, "%d", pktGo->pkt.real_payload_len);
								xapCur = xmlNewProp(xnpPacket, (xmlChar*)"data_len", (xmlChar*)buf);
								//###################################################
								//Packet의 Data 설정
								//###################################################
								pktGo->extractPayload(buf);
								xnpData = xmlNewChild(xnpPacket, NULL, (xmlChar*)"data", (xmlChar*)buf);
							}

							++messageCount;
							break;
						}
					}
				}
				if (itMessageFormat->uivCoverdMessageIdList.size()!=messageCount || itMessageFormat->uiCoverdMessage!=messageCount)
				{
					printf("Num_of_CoverdMessages : %d != checkCount : %d != \n", itMessageFormat->uivCoverdMessageIdList.size(),messageCount,itMessageFormat->uiCoverdMessage);
					g_err((char*)"printMessageFormatListWithPacketXML() : identified message count error");
				}
			}


	xmlSaveFormatFileEnc(p_cpFileName, xdpDoc, "utf-8", 1);

	xmlFreeDoc(xdpDoc);

	cTimeChecker.endClock();

	puts("printMessageFormatListWithPacketXML() : end");
	cTimeChecker.print();

	return;
}
int extractDelimiterFieldFormat(SequenceVector* p_cpFieldSequenceVector, SequenceVector* p_cpMessageSequenceVector, FieldFormatList* p_cpDelimiterFieldFormatList, bool* bDelimiterExistent)
{

	puts("extractDelimiterFieldFormat() : start");

	TimeChecker						cTimeChecker;
	FieldFormat						FieldFormatTemp;
	list<FieldFormat>::iterator		itFieldFormat;
	vector<char>::iterator			itChar;
	vector<Sequence>::iterator		itSequence;
	vector<double>::iterator		itDouble;
	vector<int>::iterator			itInt;
	list<Source>::iterator			itSource;
	list<u_int16_t>::iterator		itOffset;
	map<int, int>::iterator		itMap;

	int								iDelimiterCount;
	vector<vector<int> >			viDelimiterCountVector;	
	vector<vector<int> >			viDelimiterCountTypeVector;
	vector<int>						viIntTemp;
	vector<double>					vdDoubleTemp;
	vector<double>					vdDelimiterOffsetAverageTemp;
	vector<vector<double> >			vdDelimiterOffsetVarianceofAverage;
	vector<vector<double> >			vdDelimiterFieldFormatOffsetVariance;
	vector<double>					vdDelimiterCount;
	vector<double>					vdDelimiterVariance;
	map<int,int>					mDelimiterCountMap;


	int								iMessageSequenceSize=p_cpMessageSequenceVector->cvSequenceVector.size();

	
	int								iFieldLengthNum=0;
	int								iFieldLengthTemp=0;
	int								iPreFieldLength=0;
	int								iNextFieldLength=0;

	double							dmaxtemp;
	double							dmintemp;

	Source								cSourceTemp;
	u_int32_t							iOffset;
	bool								bFirst;

	
	cTimeChecker.startClock();

	//사전에 정의한 구분자 필드를 구분자 필드 리스트에 삽입
	for(int i=0;i<5 ;i++ )
	{
		FieldFormatTemp.reset();
		switch(i)
		{
			case 0: //|0d||0a|
					FieldFormatTemp.cContent.vcChars.push_back(0x0D);
					FieldFormatTemp.cContent.vcChars.push_back(0x0A);
					break;
			case 1: // space
					FieldFormatTemp.cContent.vcChars.push_back(0x20);
					break;
			case 2: // ;
					FieldFormatTemp.cContent.vcChars.push_back(0x3B);
					break;
			case 3: // =
					FieldFormatTemp.cContent.vcChars.push_back(0x3D);
					break;
			case 4: // :space
					FieldFormatTemp.cContent.vcChars.push_back(0x3A);
					FieldFormatTemp.cContent.vcChars.push_back(0x20);
					break;

		}
		
		//FieldFormatTemp.print();
		p_cpDelimiterFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
		
	}

//구분자필드들의 Offset 계산기
for (itFieldFormat = p_cpDelimiterFieldFormatList->clFieldFormat.begin(); itFieldFormat != p_cpDelimiterFieldFormatList->clFieldFormat.end(); ++itFieldFormat)
	{
	//	itFieldFormat->print();
		for (itSequence = p_cpMessageSequenceVector->cvSequenceVector.begin(); itSequence != p_cpMessageSequenceVector->cvSequenceVector.end(); ++itSequence)
		{
		//	itSequence->print();
			
			iOffset = 0;
			bFirst = true;
			do
			{
				iOffset = itSequence->clSequenceContentList.begin()->ctnctn(iOffset, &itFieldFormat->cContent);

				if (iOffset != -1)
				{
					//처음
					if (bFirst)
					{
						cSourceTemp.reset();
						//content Format source의 id는 content sequence의 id
						cSourceTemp.uiSourceID = itSequence->uiSequenceID;
						//content Format source의 index는 0
						cSourceTemp.uiSourceIndex = 0;
						//content Format source의 offset은 해당 content가 content sequence에 나타나는 위치, 여러번 가능
						cSourceTemp.uilSourceOffsetList.push_back(iOffset);
						
						itFieldFormat->clSourceList.push_back(cSourceTemp);
						bFirst = false;
					}
					else
					{
						itFieldFormat->clSourceList.back().uilSourceOffsetList.push_back(iOffset);
					}
				}
				iOffset++;	//gyh-check-> -1일 경우 0으로 변경, 아닐 경우에는 ctnctn할 다음 startoffset
			}
			while (iOffset != 0);
		}
		//itFieldFormat->print();
		//getchar();
	}







	int								iDelimiterFieldFormatNum=p_cpDelimiterFieldFormatList->clFieldFormat.size();
	double							dDelimiterFieldFormatVariance[iDelimiterFieldFormatNum];
	double							dDelimiterFieldFormatAverage[iDelimiterFieldFormatNum];
	double							dFieldLengthAverage[iDelimiterFieldFormatNum];
	double							dFieldLengthVariance[iDelimiterFieldFormatNum];

	
	viIntTemp.clear();
	viDelimiterCountTypeVector.clear();
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
			
			//모든 메시지 시퀀스 순회
			for(itSequence=p_cpMessageSequenceVector->cvSequenceVector.begin();itSequence!=p_cpMessageSequenceVector->cvSequenceVector.end();++itSequence)
			{
				
				iDelimiterCount=0;
	
				for(itChar=itSequence->clSequenceContentList.front().vcChars.begin(); itChar!=itSequence->clSequenceContentList.front().vcChars.end();++itChar)
				{
					
					if(itFieldFormat->cContent.vcChars.size()==1)
					{
						if(*itChar==itFieldFormat->cContent.vcChars.front())
						{
							iDelimiterCount++;
						}
				
					}
					
					else 
					{
						if(*itChar==itFieldFormat->cContent.vcChars.front()) 
						{
		
							if(*(itChar+1)==itFieldFormat->cContent.vcChars.back())
							{
								itChar++;
								iDelimiterCount++;
							}
								


						}
					}

					
				}

				viIntTemp.push_back(iDelimiterCount);
				
			}
		
			viDelimiterCountVector.push_back(viIntTemp);
			sort(viIntTemp.begin(),viIntTemp.end());
			viIntTemp.erase(unique(viIntTemp.begin(),viIntTemp.end()),viIntTemp.end());
			viDelimiterCountTypeVector.push_back(viIntTemp);
			viIntTemp.clear();
	}
	double		dEntropy=0;
	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	for(int i=0;i<p_cpDelimiterFieldFormatList->clFieldFormat.size();i++)
	{
		mDelimiterCountMap.clear();
		for(int j=0;j<viDelimiterCountTypeVector[i].size();j++)
			mDelimiterCountMap.insert(make_pair(viDelimiterCountTypeVector[i][j],0));
		for(int j=0;j<viDelimiterCountVector[i].size();j++)
		{
			for(itMap=mDelimiterCountMap.begin();itMap!=mDelimiterCountMap.end();++itMap)
			{
				if(viDelimiterCountVector[i][j]==itMap->first)
				{
					itMap->second=itMap->second + 1;
				}
			}
			
		}
		dEntropy=0;
		
		for(itMap=mDelimiterCountMap.begin();itMap!=mDelimiterCountMap.end();++itMap)
		{
			dEntropy+=((double)itMap->second / (double)viDelimiterCountVector[i].size() * logB((double)itMap->second / (double)viDelimiterCountVector[i].size(),2.0));
		}
		dEntropy=-dEntropy;
		itFieldFormat->dDelimiterEntropy=dEntropy;
		itFieldFormat++;
		printf("Delimiter Entropy = %lf\n",dEntropy);
		//getchar();

	}
	

double temp=0;

//메시지마다 구분자필드로 구분된 필드들의 길이 분산의 평균
/*
for(int k=0; k<p_cpFieldFormatList->clFieldFormat.size(); k++)
{
	
	printf("--------------------------%d------------------------------ \n",k);

	temp=0;
		
		for(itInt=viFieldLengthVector[k].begin();itInt!=viFieldLengthVector[k].end();++itInt)
		{
	
			temp+=(double)*itInt;
			
		}

	
	
	dFieldLengthAverage[k]=(double)temp/(double)viFieldLengthVector[k].size();
	printf("FieldLengthAverage[%d] = %lf\n",k,dFieldLengthAverage[k]);
	//getchar();

}
printf("\n\n");

	maxtemp=viFieldLength[0];
	mintemp=viFieldLength[0];

	for(itInt=viFieldLength.begin();itInt!=viFieldLength.end();++itInt)
	{
		if(*itInt > maxtemp)
			maxtemp=*itInt;
		if(*itInt < mintemp)
			mintemp=*itInt;
	}


	for(int i=0; i<p_cpFieldFormatList->clFieldFormat.size(); i++)
	{
		temp=0;

		for(itInt=viFieldLengthVector[i].begin();itInt!=viFieldLengthVector[i].end();++itInt)
		{
	
			temp+=(((double)*itInt-dFieldLengthAverage[i]) * ((double)*itInt-dFieldLengthAverage[i]));
			
		}

		dFieldLengthVariance[i]=temp/(double)(viFieldLengthVector[i].size()-1);

		printf("dFieldLengthVariance[%d] = %lf\n",i,dFieldLengthVariance[i]);
	}


*/







	for(int i=0; i<p_cpDelimiterFieldFormatList->clFieldFormat.size(); i++)
	{
		//printf("--------------------------%d------------------------------ \n",i);
		temp=0;
		int j=0;
		int imaxtemp=viDelimiterCountVector[i][j];
		int imintemp=viDelimiterCountVector[i][j];
		/*
		
		for(j=1;j<iMessageSequenceSize;j++)
		{
			
			if(imaxtemp<viDelimiterCountVector[i][j])
				imaxtemp=viDelimiterCountVector[i][j];
			if(imintemp>viDelimiterCountVector[i][j])
				imintemp=viDelimiterCountVector[i][j];

		}	
		printf("max = %d \nmin = %d\n\n",imaxtemp,imintemp);
		for(j=0;j<iMessageSequenceSize;j++)
		{
			vdDelimiterCount.push_back((double)(viDelimiterCountVector[i][j]-imintemp)/(double)(imaxtemp-imintemp));
	
		}

		for(j=0;j<iMessageSequenceSize;j++)
		{
			temp+=vdDelimiterCount[j];
			if(vdDelimiterCount[j]>1 || vdDelimiterCount[j]<0)
				printf("%lf\n",vdDelimiterCount[j]);
		}
		dDelimiterFieldFormatAverage[i]=temp/(double)iMessageSequenceSize;
		printf("avg = %lf\n\n",dDelimiterFieldFormatAverage[i]);
		temp=0;
		for(j=0;j<iMessageSequenceSize;j++)
		{
			temp+=((vdDelimiterCount[j]-dDelimiterFieldFormatAverage[i]) * (vdDelimiterCount[j]-dDelimiterFieldFormatAverage[i]));
			//printf("%lf\n",((vdDelimiterCount[j]-dDelimiterFieldFormatAverage[i]) * (vdDelimiterCount[j]-dDelimiterFieldFormatAverage[i])));
			//getchar();
		}
		
		dDelimiterFieldFormatVariance[i]=temp/(double)(iMessageSequenceSize-1);
		vdDelimiterCount.clear();
*/

		

		for(j=0;j<iMessageSequenceSize;j++)
		{
	
			temp+=viDelimiterCountVector[i][j];
			
		}
		dDelimiterFieldFormatAverage[i]=temp/(double)iMessageSequenceSize;
		//printf("avg = %lf\n\n",dDelimiterFieldFormatAverage[i]);
		temp=0;
		for(j=0;j<iMessageSequenceSize;j++)
		{
			temp+=((viDelimiterCountVector[i][j]-dDelimiterFieldFormatAverage[i]) * (viDelimiterCountVector[i][j]-dDelimiterFieldFormatAverage[i]));
		
		}
		
		dDelimiterFieldFormatVariance[i]=temp/(double)(iMessageSequenceSize);
		vdDelimiterCount.clear();




	}

	printf("\n\n");
	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	for(int i=0; i<p_cpDelimiterFieldFormatList->clFieldFormat.size(); i++)
	{
		
			printf("dDelimiterFieldFormatVariance[%d]=%lf\n\n",i,dDelimiterFieldFormatVariance[i]);
			itFieldFormat->dDelimiterFieldFormatCountVariance=dDelimiterFieldFormatVariance[i];
			itFieldFormat++;
		
		
	}


	//메시지마다 구분자필드 Offset 평균과 분산
	double		dVarianceTemp=0;
	int itemp=0;
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		for(itSource=itFieldFormat->clSourceList.begin();itSource!=itFieldFormat->clSourceList.end();++itSource)
		{
			vdDoubleTemp.clear();
			temp=0;
			dVarianceTemp=0;
			dmintemp=(double)itSource->uilSourceOffsetList.front();
			dmaxtemp=(double)itSource->uilSourceOffsetList.front();
			for(itOffset=itSource->uilSourceOffsetList.begin();itOffset!=itSource->uilSourceOffsetList.end();++itOffset)
			{
				if(isnan(*itOffset))
				{
					printf("Offset Error\n");
					getchar();
				}
				vdDoubleTemp.push_back((double)*itOffset);
				
				if(dmintemp>*itOffset)
					dmintemp=(double)*itOffset;
				if(dmaxtemp<*itOffset)
					dmaxtemp=(double)*itOffset;
				//printf("%d\n",*itOffset);
				temp+=(double)*itOffset;
			}
			//getchar();
			temp=temp/(double)vdDoubleTemp.size();
			vdDelimiterOffsetAverageTemp.push_back(temp);


			//min-max 정규화 -> 에러발생 왜?

			//에러 발생 지점
			
			//printf("maxtemp = %lf mintemp = %lf\n",dmaxtemp,dmintemp);
			for(int i=0;i<vdDoubleTemp.size();i++)
			{
				//vdDoubleTemp[i]=(vdDoubleTemp[i]-dmintemp)/(dmaxtemp-dmintemp);
				temp+=vdDoubleTemp[i];
				if(isnan(vdDoubleTemp[i]))
				{
					printf("vdDoubleTemp[i] Error\n");
					getchar();
				}
				

			}
			

			temp=temp/(double)vdDoubleTemp.size();
			if(isnan(temp))
			{
				printf("Average Error\n");

			}
				
			//정규화된 값들에 대한 분산값
			for(int i=0;i<vdDoubleTemp.size();i++)
			{
				dVarianceTemp+=(vdDoubleTemp[i]-temp)*(vdDoubleTemp[i]-temp);
				
			}
			if(isnan(dVarianceTemp))
			{
				printf("dVarianceTemp1 = %lf\n",dVarianceTemp);
				getchar();
			}
			itemp=vdDoubleTemp.size();
			dVarianceTemp=dVarianceTemp/(double)itemp;
			vdDelimiterVariance.push_back(dVarianceTemp);
			if(isnan(dVarianceTemp))
			{
					printf("dVarianceTemp2 = %lf\n",dVarianceTemp);
				getchar();
			}
		
		}
		vdDelimiterFieldFormatOffsetVariance.push_back(vdDelimiterVariance);
		vdDelimiterVariance.clear();
		vdDelimiterOffsetVarianceofAverage.push_back(vdDelimiterOffsetAverageTemp);
		vdDelimiterOffsetAverageTemp.clear();
	}


	//getchar();
	//메시지마다 구분자 필드들의 Offset 분산의 평균 구하기
	/*
	for(int i=0;i<5;i++)
	{
		temp=0;
		int j=0;
		for(j=0;j<vdDelimiterFieldFormatOffsetVariance[i].size();j++)
		{
			temp+=vdDelimiterFieldFormatOffsetVariance[i][j];
			if(isnan(vdDelimiterFieldFormatOffsetVariance[i][j]))
			{
				printf("%lf\n",vdDelimiterFieldFormatOffsetVariance[i][j]);
				getchar();
			}
		}
	
		temp=temp/(double)j;
		//printf("size = %d\n",vdDelimiterFieldFormatOffsetVariance[i].size());
		printf("Average of Variance = %lf\n\n",temp);
	}
	*/

	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	//메시지마다 구분자 필드들의 Offset 평균의 분산
	for(int i=0;i<5;i++)
	{
		temp=0;
		int j=0;
		for(j=0;j<vdDelimiterOffsetVarianceofAverage[i].size();j++)
		{
			temp+=vdDelimiterOffsetVarianceofAverage[i][j];
		}
		temp=temp/vdDelimiterOffsetVarianceofAverage[i].size();

		for(j=0;j<vdDelimiterOffsetVarianceofAverage[i].size();j++)
		{
			vdDelimiterOffsetVarianceofAverage[i][j]=(vdDelimiterOffsetVarianceofAverage[i][j]-temp)*(vdDelimiterOffsetVarianceofAverage[i][j]-temp);
		}
		temp=temp/(double)vdDelimiterOffsetVarianceofAverage[i].size();
		
		itFieldFormat->dDelimiterOffsetVarianceofAverage=temp;
		//printf("Variance of Average = %lf\t%lf\n\n",temp,itFieldFormat->dDelimiterOffsetVarianceofAverage);
		itFieldFormat++;
		printf("Variance of Average = %lf\n\n",temp);
		

	}
	
	double dCountVariancemintemp=100;
	double dOffsetVariancemaxtemp=100;
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->dDelimiterFieldFormatCountVariance<dCountVariancemintemp)
			dCountVariancemintemp=itFieldFormat->dDelimiterFieldFormatCountVariance;
		if(itFieldFormat->dDelimiterOffsetVarianceofAverage>dOffsetVariancemaxtemp)
			dOffsetVariancemaxtemp=itFieldFormat->dDelimiterOffsetVarianceofAverage;
	}
	
	int		DelimiterCandidateCount=0;
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		/*
		if(itFieldFormat->dDelimiterFieldFormatCountVariance<15 && (0.2<itFieldFormat->dDelimiterOffsetVarianceofAverage && itFieldFormat->dDelimiterOffsetVarianceofAverage<0.4))
		{
			itFieldFormat->bDelimiterFlag=true;
			//itFieldFormat->print();
			*bDelimiterExistent=true;
			break;
		}
		*/
		if(itFieldFormat->dDelimiterEntropy<=3.2 && (0.1<itFieldFormat->dDelimiterOffsetVarianceofAverage && itFieldFormat->dDelimiterOffsetVarianceofAverage<2.5))
		{
			if(itFieldFormat->dDelimiterEntropy + itFieldFormat->dDelimiterOffsetVarianceofAverage <= 3.5)
			{
				itFieldFormat->bDelimiterFlag=true;
				DelimiterCandidateCount++;
				//itFieldFormat->print();
				//getchar();
				*bDelimiterExistent=true;

			}
		}
	}

	double		dDelimiterEntropyMinValue=100;
	if(DelimiterCandidateCount>=2)
	{
		for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
		{
			if(itFieldFormat->bDelimiterFlag)
			{
				if(dDelimiterEntropyMinValue>=itFieldFormat->dDelimiterEntropy)
				{
					dDelimiterEntropyMinValue=itFieldFormat->dDelimiterEntropy;
					break;
				}

			}
		}

		for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
		{
			if(itFieldFormat->dDelimiterEntropy!=dDelimiterEntropyMinValue)
			{
				itFieldFormat->bDelimiterFlag=false;
			}
			else
			{
				//itFieldFormat->print();
				//getchar();
			}
		}
	}

	

	
	puts("extractDelimiterFieldFormat() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
	getchar();
	return p_cpDelimiterFieldFormatList->clFieldFormat.size();

}
//###########################################################################################################################
int splitFieldFormatbyDelimiter(SequenceVector* p_cpFieldSequenceVector, SequenceVector* p_cpMessageSequenceVector, FieldFormatList* p_cpSplitedFieldFormatList, FieldFormatList* p_cpDelimiterFieldFormatList)
{
	list<FieldFormat>::iterator		itFieldFormat;
	vector<char>::iterator			itChar;
	vector<Sequence>::iterator		itSequence;
	FieldFormat						FieldFormatTemp;
	vector<char>					Delimiter;
	int								FieldFormatID=0;
	Source							SourceTemp;
	bool							deleteFlag;
	u_int16_t								iOffset=0;
	puts("splitFieldFormatbyDelimiter() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();
	Delimiter.clear();


	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->bDelimiterFlag)
		{
			Delimiter.assign(itFieldFormat->cContent.vcChars.begin(), itFieldFormat->cContent.vcChars.end());
			break;

			//printf("%d",itFieldFormat->cContent.vcChars.size());
			//getchar();
		}
			
	}
	bool				bFirst=true;
	bool				bBody=false;
	
	double				mintemp=0.5;
	int					counttemp=0;
	for(itSequence=p_cpMessageSequenceVector->cvSequenceVector.begin();itSequence!=p_cpMessageSequenceVector->cvSequenceVector.end();++itSequence)
	{
		FieldFormatTemp.reset();
		iOffset=0;
		
		SourceTemp.reset();
		counttemp=0;
		for(itChar=itSequence->clSequenceContentList.front().vcChars.begin();itChar!=itSequence->clSequenceContentList.front().vcChars.end();++itChar,iOffset++)
		{
			if(bFirst)
			{
				SourceTemp.uilSourceOffsetList.push_back(iOffset);
				bFirst=false;
			}
			/*
				if(itSequence->clSequenceContentList.front().vcChars.size()*0.08<=iOffset)
				{
					bBody=true;
					break;
					
				}
				
				if(itSequence->clSequenceContentList.front().uiDirection==DIREC_REQUEST)
					continue;
			*/
			if(Delimiter.size()==2)
			{
				if(*itChar==Delimiter.front() && *(itChar+1)==Delimiter.back())
				{
					if (((*(itChar+2) >=' ') && (*(itChar+2) <= '~')) && (*(itChar+2) != ';') && (*(itChar+2) != '\\') && (*(itChar+2) != '"') && (*(itChar+2) != '|')&& (*(itChar+2) != '<')&& (*(itChar+2) != '>'))
					{
						++itChar;
						FieldFormatTemp.cContent.uiDirection=itSequence->clSequenceContentList.front().uiDirection;
						FieldFormatTemp.cContent.cContentHeader.uiProtocol=itSequence->clSequenceContentList.front().cContentHeader.uiProtocol;
						FieldFormatTemp.uivIdentifiedMessageIdList.push_back(itSequence->uiSequenceID);
						SourceTemp.uiSourceID=itSequence->uiSequenceID;
						FieldFormatTemp.clSourceList.push_back(SourceTemp);
						SourceTemp.reset();
						SourceTemp.uilSourceOffsetList.push_back(iOffset+3);
						p_cpSplitedFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
						//FieldFormatTemp.print();
						//getchar();
						
						FieldFormatTemp.reset();
						continue;
					}
					else
					{
						++itChar;
					FieldFormatTemp.cContent.uiDirection=itSequence->clSequenceContentList.front().uiDirection;
					FieldFormatTemp.cContent.cContentHeader.uiProtocol=itSequence->clSequenceContentList.front().cContentHeader.uiProtocol;
					FieldFormatTemp.uivIdentifiedMessageIdList.push_back(itSequence->uiSequenceID);
					SourceTemp.uiSourceID=itSequence->uiSequenceID;
					FieldFormatTemp.clSourceList.push_back(SourceTemp);
					SourceTemp.reset();
					SourceTemp.uilSourceOffsetList.push_back(iOffset+3);
					p_cpSplitedFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
					
					//FieldFormatTemp.print();
					//getchar();
					FieldFormatTemp.reset();
					break;

					}
			
				}
				
			}

			else
			{
				if(*itChar==Delimiter.front())
				{
					FieldFormatTemp.cContent.uiDirection=itSequence->clSequenceContentList.front().uiDirection;
					FieldFormatTemp.cContent.cContentHeader.uiProtocol=itSequence->clSequenceContentList.front().cContentHeader.uiProtocol;
					FieldFormatTemp.uivIdentifiedMessageIdList.push_back(itSequence->uiSequenceID);
					//Source ID 는 메시지 시퀀스 ID, SourceOffsetList에 메시지 시퀀스 내 Offset 
					SourceTemp.uiSourceID=itSequence->uiSequenceID;
				
					FieldFormatTemp.clSourceList.push_back(SourceTemp);
					SourceTemp.reset();
					SourceTemp.uilSourceOffsetList.push_back(iOffset+2);
				
					FieldFormatTemp.reset();
					continue;

				}

			}
			
			
			FieldFormatTemp.cContent.vcChars.push_back(*itChar);
			
		}

		/*
		if(itSequence->clSequenceContentList.front().uiDirection==DIREC_RESPONSE)
			printf("Message Direction = Response\t");
		else
			printf("Message Direction = Request\t");

		printf("Main Delimiter Count = %d\n",counttemp);
		getchar();
		*/
/*
		FieldFormatTemp.cContent.uiDirection=itSequence->clSequenceContentList.front().uiDirection;
		FieldFormatTemp.cContent.cContentHeader.uiProtocol=itSequence->clSequenceContentList.front().cContentHeader.uiProtocol;
		FieldFormatTemp.uivIdentifiedMessageIdList.push_back(itSequence->uiSequenceID);
		SourceTemp.uiSourceID=itSequence->uiSequenceID;
		
		FieldFormatTemp.clSourceList.push_back(SourceTemp);
		SourceTemp.reset();
		p_cpSplitedFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
		//FieldFormatTemp.print();
		//getchar();
		FieldFormatTemp.reset();
		//getchar();
		
*/
	}



	//printf("min value = %lf",mintemp);



	for (itFieldFormat=p_cpSplitedFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpSplitedFieldFormatList->clFieldFormat.end() ;++itFieldFormat )
	{
		itFieldFormat->uiFieldFormatIDArrange=FieldFormatID++;
	}



	puts("splitFieldFormatbyDelimiter() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
	return p_cpSplitedFieldFormatList->clFieldFormat.size();

}
//###########################################################################################################################
int deDuplicationFieldFormat(FieldFormatList* p_cpFieldFormatList)
{
	int FieldFormatID=0;
	bool						flag=false;
	list<FieldFormat>::iterator itFieldFormat;
	list<FieldFormat>::iterator itFieldFormat2;
	vector<char>::iterator		itChar;
	vector<char>::iterator		itChar2;

	vector<u_int32_t>::iterator	itUint;
	puts("deDuplicationFieldFormat() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();
	
	//최소필드길이 이하 제거
	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
	{
		if(itFieldFormat->cContent.vcChars.size()<2)
			p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
		else
			itFieldFormat++;
	}

	itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();
	itFieldFormat2=p_cpFieldFormatList->clFieldFormat.begin();
	itFieldFormat2++;
	
	while(itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end())
	{
		while(itFieldFormat2!=p_cpFieldFormatList->clFieldFormat.end())
		{
			flag=false;
			if(itFieldFormat->cContent.vcChars.size()!=itFieldFormat2->cContent.vcChars.size())
			{
				itFieldFormat2++;
				
			}
			else
			{
			
				itChar=itFieldFormat->cContent.vcChars.begin();
				itChar2=itFieldFormat2->cContent.vcChars.begin();
				while(itChar!=itFieldFormat->cContent.vcChars.end() && itChar2!=itFieldFormat2->cContent.vcChars.end())
				{
					if(*itChar!=*itChar2)
					{
						//printf("Different Content\n");
						flag=true;
						break;
					}
				

					itChar++;
					itChar2++;
				}

				if(!flag)
				{
					if(itFieldFormat->cContent.uiDirection != itFieldFormat2->cContent.uiDirection)
					{
						itFieldFormat2++;
					}
					else
					{
						for(itUint=itFieldFormat2->uivIdentifiedMessageIdList.begin();itUint!=itFieldFormat2->uivIdentifiedMessageIdList.end();++itUint)
						{
							itFieldFormat->uivIdentifiedMessageIdList.push_back(*itUint);
						}
						p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat2++);
					}

					
				}
				else
				{
					itFieldFormat2++;
				}
	
			}

		}
		itFieldFormat++;
		itFieldFormat2=++itFieldFormat;
		itFieldFormat--;
		
		

	}

	//p_cpSplitedFieldFormatList->clFieldFormat.sort(FieldFormat::CompareContentLength());
	//p_cpSplitedFieldFormatList->clFieldFormat.sort(FieldFormat::CompareContentDirec());
	//p_cpSplitedFieldFormatList->clFieldFormat.unique(FieldFormat::CompareContent());





	itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();
	itFieldFormat2=p_cpFieldFormatList->clFieldFormat.begin();
	itFieldFormat2++;
	int			iMatchCount=0;
	int			iFieldSizeCount=0;
	int			iFieldSize=0;
	
	//한 필드포맷에 두개의 다른 필드포맷이 포함되어있고 가운데 어떤 문자가 있다면 삭제
	while(itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end())
	{
		iMatchCount=0;
		iFieldSize=0;
		while(itFieldFormat2!=p_cpFieldFormatList->clFieldFormat.end())
		{
			
			/*
			if(itFieldFormat->cContent.uiDirection != itFieldFormat2->cContent.uiDirection)
			{
				itFieldFormat2++;
				continue;
			}
			*/
			//비교대상 itFieldFormat2가 1보다길이가 크다면 2는 1에 포함될 수 없으므로 
			
			if(itFieldFormat->cContent.vcChars.size()>itFieldFormat2->cContent.vcChars.size())
			{
				
				iFieldSizeCount=0;
				//itChar=itFieldFormat->cContent.vcChars.begin();
				itChar2=itFieldFormat2->cContent.vcChars.begin();

					for(itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end();++itChar)
					{
						if(*itChar2 == *itChar)
						{
							iFieldSizeCount++;
							itChar2++;
						}

					}
					/*
					itFieldFormat->print();
					itFieldFormat2->print();
					printf("iFieldSizeCount = %d, itFieldFormat2 Size = %d\n",iFieldSizeCount,itFieldFormat2->cContent.vcChars.size());
					getchar();
					*/

					//itFieldFormat2가 1에 완벽히 포함된다면
					if(iFieldSizeCount == itFieldFormat2->cContent.vcChars.size())
					{
						iMatchCount++;
						iFieldSize+=iFieldSizeCount;
						//itFieldFormat2->print();
						//getchar();
					}
						
						
					
			} //if
			

			
			itFieldFormat2++;

		}//itFieldFormat2

		if(iMatchCount==2 && itFieldFormat->cContent.vcChars.size()-1 == iFieldSize)
		{
			itFieldFormat->bDeleteFlag=true;
			//itFieldFormat->print();
			//getchar();
			
		}
		itFieldFormat++;
		itFieldFormat2=p_cpFieldFormatList->clFieldFormat.begin();

	}//itFieldFormat








	for (itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end() ; )
	{
		if(itFieldFormat->bDeleteFlag)
		{
			p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
		}
		else
			itFieldFormat++;
	}










	FieldFormatID=0;
	p_cpFieldFormatList->clFieldFormat.sort(FieldFormat::CompareContentLength());
	p_cpFieldFormatList->clFieldFormat.sort(FieldFormat::CompareContentDirec());

	for (itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end() ;++itFieldFormat )
	{
		itFieldFormat->uiFieldFormatIDArrange=FieldFormatID++;
		sort(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
		unique(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());

	
		
	}


	puts("deDuplicationFieldFormat() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
	return p_cpFieldFormatList->clFieldFormat.size();
}
//###############################################################################################################################################
int extractSubDelimiterFieldFormat(FieldFormatList* p_cpFieldFormatList, FieldFormatList* p_cpSplitedFieldFormatList, FieldFormatList* p_cpDelimiterFieldFormatList, bool* bSubDelimiterExistent)
{
	list<FieldFormat>::iterator			itFieldFormat;
	list<FieldFormat>::iterator			itFieldFormat2;
	vector<char>::iterator				itChar;
	vector<Sequence>::iterator		itSequence;
	vector<double>::iterator		itDouble;
	vector<int>::iterator			itInt;
	list<Source>::iterator			itSource;
	list<u_int16_t>::iterator		itOffset;

	vector<Content>						StringTemp;
	Content								ContentTemp;

	vector<int>							viIntTemp;
	vector<vector<int> >				viDelimiterCountVector;	
	vector<double>						vdDelimiterCount;
	vector<double>						vdDelimiterVariance;

	
	vector<double>					vdDoubleTemp;
	vector<double>					vdDelimiterOffsetAverageTemp;
	vector<vector<double> >			vdDelimiterOffsetVarianceofAverage;
	vector<vector<double> >			vdDelimiterFieldFormatOffsetVariance;

	Source								cSourceTemp;
	u_int32_t							iOffset;
	bool								bFirst;
	int									i=0;
	int									iDelimiterCount=0;
	double							dmaxtemp;
	double							dmintemp;

	puts("extractSubDelimiterFieldFormat() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();

	ContentTemp.reset();
	for(itFieldFormat=p_cpSplitedFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpSplitedFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		ContentTemp.vcChars.assign(itFieldFormat->cContent.vcChars.begin(),itFieldFormat->cContent.vcChars.end());
		StringTemp.push_back(ContentTemp);
		ContentTemp.reset();

	}
	int								iFieldFormatSize=StringTemp.size();
	//모든 Sub Delimiter 후보 순회
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		//모든 SplitedFieldFormat 순회
		for(i=0;i<StringTemp.size();i++)
		{
			iDelimiterCount=0;
			for (itChar=StringTemp[i].vcChars.begin();itChar!=StringTemp[i].vcChars.end();++itChar )
			{
				if(itFieldFormat->cContent.vcChars.size()==1)
				{
					if(*itChar==itFieldFormat->cContent.vcChars.front())
					{
						iDelimiterCount++;
					
					}
			
				}
				
				else
				{
					if(*itChar==itFieldFormat->cContent.vcChars.front())
					{
	
						if(*(itChar+1)==itFieldFormat->cContent.vcChars.back())
						{
							itChar++;
							iDelimiterCount++;

						}
							
					}
				}

			}// Char

			viIntTemp.push_back(iDelimiterCount);

		}//SplitedFieldFormat
		viDelimiterCountVector.push_back(viIntTemp);
		viIntTemp.clear();

	}//Sub Delimiter


	double temp=0;
	
	double							dDelimiterFieldFormatVariance[iFieldFormatSize];
	double							dDelimiterFieldFormatAverage[iFieldFormatSize];
	//Count 평균의 분산값
	for(int i=0; i<p_cpDelimiterFieldFormatList->clFieldFormat.size(); i++)
	{
		//printf("--------------------------%d------------------------------ \n",i);
		temp=0;
		int j=0;
		int imaxtemp=viDelimiterCountVector[i][j];
		int imintemp=viDelimiterCountVector[i][j];

		for(j=0;j<iFieldFormatSize;j++)
		{
	
			temp+=viDelimiterCountVector[i][j];
			
		}
		dDelimiterFieldFormatAverage[i]=temp/(double)iFieldFormatSize;
		//printf("avg = %lf\n\n",dDelimiterFieldFormatAverage[i]);
		temp=0;
		for(j=0;j<iFieldFormatSize;j++)
		{
			temp+=((viDelimiterCountVector[i][j]-dDelimiterFieldFormatAverage[i]) * (viDelimiterCountVector[i][j]-dDelimiterFieldFormatAverage[i]));
		
		}
		
		dDelimiterFieldFormatVariance[i]=temp/(double)(iFieldFormatSize);
		vdDelimiterCount.clear();




	}
	printf("\n\n");
	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	for(int i=0; i<p_cpDelimiterFieldFormatList->clFieldFormat.size(); i++)
	{
		printf("dDelimiterFieldFormatVariance[%d]=%lf\n",i,dDelimiterFieldFormatVariance[i]);
		itFieldFormat->dSubDelimiterFieldFormatCountVariance=dDelimiterFieldFormatVariance[i];
		itFieldFormat++;
	}
	printf("\n\n");


	for(itFieldFormat = p_cpDelimiterFieldFormatList->clFieldFormat.begin(); itFieldFormat != p_cpDelimiterFieldFormatList->clFieldFormat.end(); ++itFieldFormat)
	{
		for(itFieldFormat2=p_cpSplitedFieldFormatList->clFieldFormat.begin();itFieldFormat2!=p_cpSplitedFieldFormatList->clFieldFormat.end();++itFieldFormat2)
		{
			iOffset=0;
			bFirst=true;
			
			do{
				iOffset=itFieldFormat2->cContent.ctnctn(iOffset, &itFieldFormat->cContent);
				if(iOffset!= -1)
				{
					if(bFirst)
					{
						cSourceTemp.reset();
						//clSubDelimiterSourceList 에서 SourceID는 Splited FieldFormat ID를 의미
						cSourceTemp.uiSourceID=itFieldFormat2->uiFieldFormatIDArrange;
						cSourceTemp.uiSourceIndex=0;
						cSourceTemp.uilSourceOffsetList.push_back(iOffset);
						itFieldFormat->clSubDelimiterSourceList.push_back(cSourceTemp);
						bFirst=false;

					}
					else
					{
						itFieldFormat->clSubDelimiterSourceList.back().uilSourceOffsetList.push_back(iOffset);
				
					}


				}

				iOffset++;

			}
			while (iOffset!=0);
			
		}

	}

	//SplitedFieldFormat마다 서브구분자필드 Offset 평균과 분산
	double		dVarianceTemp=0;
	int itemp=0;
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		for(itSource=itFieldFormat->clSubDelimiterSourceList.begin();itSource!=itFieldFormat->clSubDelimiterSourceList.end();++itSource)
		{
			vdDoubleTemp.clear();
			temp=0;
			dVarianceTemp=0;
			dmintemp=(double)itSource->uilSourceOffsetList.front();
			dmaxtemp=(double)itSource->uilSourceOffsetList.front();
			for(itOffset=itSource->uilSourceOffsetList.begin();itOffset!=itSource->uilSourceOffsetList.end();++itOffset)
			{
				if(isnan(*itOffset))
				{
					printf("Offset Error\n");
					getchar();
				}
				vdDoubleTemp.push_back((double)*itOffset);
				
				if(dmintemp>*itOffset)
					dmintemp=(double)*itOffset;
				if(dmaxtemp<*itOffset)
					dmaxtemp=(double)*itOffset;
				//printf("%d\n",*itOffset);
				temp+=(double)*itOffset;
			}
			//getchar();
			temp=temp/(double)vdDoubleTemp.size();
			vdDelimiterOffsetAverageTemp.push_back(temp);


			//min-max 정규화 -> 에러발생 왜?

			//에러 발생 지점
			
			//printf("maxtemp = %lf mintemp = %lf\n",dmaxtemp,dmintemp);
			for(int i=0;i<vdDoubleTemp.size();i++)
			{
				//vdDoubleTemp[i]=(vdDoubleTemp[i]-dmintemp)/(dmaxtemp-dmintemp);
				temp+=vdDoubleTemp[i];
				if(isnan(vdDoubleTemp[i]))
				{
					printf("vdDoubleTemp[i] Error\n");
					getchar();
				}
				

			}
			

			temp=temp/(double)vdDoubleTemp.size();
			if(isnan(temp))
			{
				printf("Average Error\n");

			}
				
			//정규화된 값들에 대한 분산값
			for(int i=0;i<vdDoubleTemp.size();i++)
			{
				dVarianceTemp+=(vdDoubleTemp[i]-temp)*(vdDoubleTemp[i]-temp);
				
			}
			if(isnan(dVarianceTemp))
			{
				printf("dVarianceTemp1 = %lf\n",dVarianceTemp);
				getchar();
			}
			itemp=vdDoubleTemp.size();
			dVarianceTemp=dVarianceTemp/(double)itemp;
			vdDelimiterVariance.push_back(dVarianceTemp);
			if(isnan(dVarianceTemp))
			{
					printf("dVarianceTemp2 = %lf\n",dVarianceTemp);
				getchar();
			}
		
		}
		vdDelimiterFieldFormatOffsetVariance.push_back(vdDelimiterVariance);
		vdDelimiterVariance.clear();
		vdDelimiterOffsetVarianceofAverage.push_back(vdDelimiterOffsetAverageTemp);
		vdDelimiterOffsetAverageTemp.clear();
		
	}


	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	//SplitedFieldFormat마다 서브구분자 필드들의 Offset 평균의 분산
	for(int i=0;i<5;i++)
	{
		temp=0;
		int j=0;
		if(vdDelimiterOffsetVarianceofAverage[i].size()<=0)
		{

			itFieldFormat++;
			printf("It is not Sub Delimiter\n\n");
		}
		else
		{
			for(j=0;j<vdDelimiterOffsetVarianceofAverage[i].size();j++)
			{
				temp+=vdDelimiterOffsetVarianceofAverage[i][j];
			}
			temp=temp/vdDelimiterOffsetVarianceofAverage[i].size();

			for(j=0;j<vdDelimiterOffsetVarianceofAverage[i].size();j++)
			{
				vdDelimiterOffsetVarianceofAverage[i][j]=(vdDelimiterOffsetVarianceofAverage[i][j]-temp)*(vdDelimiterOffsetVarianceofAverage[i][j]-temp);
			}
			temp=temp/(double)vdDelimiterOffsetVarianceofAverage[i].size();
			itFieldFormat->dSubDelimiterOffsetVarianceofAverage=temp;
			
			printf("Variance of Average = %lf\t%lf\n\n",temp,itFieldFormat->dSubDelimiterOffsetVarianceofAverage);
			itFieldFormat++;

		}


	}
	
	double dCountVariancemintemp=100;
	double dOffsetVariancemintemp=100;

	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->dSubDelimiterFieldFormatCountVariance==0 && itFieldFormat->dSubDelimiterOffsetVarianceofAverage==0)
			continue;
		if(itFieldFormat->dSubDelimiterFieldFormatCountVariance<dCountVariancemintemp && itFieldFormat->dSubDelimiterFieldFormatCountVariance!=0 )
			dCountVariancemintemp=itFieldFormat->dSubDelimiterFieldFormatCountVariance;
		if(itFieldFormat->dSubDelimiterOffsetVarianceofAverage<dOffsetVariancemintemp && itFieldFormat->dSubDelimiterOffsetVarianceofAverage != 0 )
			dOffsetVariancemintemp=itFieldFormat->dSubDelimiterOffsetVarianceofAverage;
	}

	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->dSubDelimiterFieldFormatCountVariance==0 || itFieldFormat->dSubDelimiterOffsetVarianceofAverage==0)
			continue;
		else
		{
			if(itFieldFormat->dSubDelimiterFieldFormatCountVariance<7.3 && itFieldFormat->dSubDelimiterOffsetVarianceofAverage==dOffsetVariancemintemp)
			{
				itFieldFormat->bSubDelimiterFlag=true;
				*bSubDelimiterExistent=true;
				//itFieldFormat->print();
				//getchar();
				break;
			}

		}

	}


	puts("extractSubDelimiterFieldFormat() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();



	return p_cpFieldFormatList->clFieldFormat.size();
}
//##########################################################################################################################
int splitFieldFormatbySubDelimiter(FieldFormatList* p_cpFieldFormatList, FieldFormatList* p_cpSplitedFieldFormatList, FieldFormatList* p_cpDelimiterFieldFormatList,  bool* p_cpSomeRequestMessageSubDelimiterExistent, bool* p_cpSomeResponseMessageSubDelimiterExistent )
{
	list<FieldFormat>::iterator		itFieldFormat;
	vector<char>::iterator			itChar;
	vector<Sequence>::iterator		itSequence;
	FieldFormat						FieldFormatTemp;
	vector<char>					Delimiter;
	int								FieldFormatID=0;
	int								iOffset=0;
	bool							flag=false;
	bool							deleteFlag;
	puts("splitFieldFormatbySubDelimiter() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();
	Delimiter.clear();

	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->bSubDelimiterFlag)
		{
			Delimiter.assign(itFieldFormat->cContent.vcChars.begin(), itFieldFormat->cContent.vcChars.end());
			//itFieldFormat->print();
			//getchar();
			break;

			//printf("%d",itFieldFormat->cContent.vcChars.size());
			
		}
			
	}

	int		iPositionCount=0;
	bool	EndofSubDelimiter=false;
	for(itFieldFormat=p_cpSplitedFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpSplitedFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		FieldFormatTemp.reset();
		iOffset=0;
		iPositionCount=0;
		//itFieldFormat->print();
		//getchar();
	
		for(itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end();++itChar)
		{
			flag=false;
			iOffset++;
			
			if(Delimiter.size()==2)
			{
				if(*itChar==Delimiter.front() && *(itChar+1)==Delimiter.back())
				{
					if(*(itChar-1)==Delimiter.back())
						continue;
					//if (((*(itChar+2) >=' ') && (*(itChar+2) <= '~')) && (*(itChar+2) != ';') && (*(itChar+2) != '\\') && (*(itChar+2) != '"') && (*(itChar+2) != '|')&& (*(itChar+2) != '<')&& (*(itChar+2) != '>'))
					//{
						if(0<iOffset&&iOffset<35)
						{

							++itChar;
							FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
							FieldFormatTemp.iFieldFormatPositioninString=iPositionCount++;
							//FieldFormatTemp.uiFieldFormatIDArrange=FieldFormatID++;
							FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
							//FieldFormatTemp.bSubDelimiterExistent=true;
							FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
							p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
						
							FieldFormatTemp.reset();
							flag=true;
							break;
						}

					//}
					

				}
				
			}

			else
			{
				if(*itChar==Delimiter.front())
				{
					if(*(itChar-1)==Delimiter.front())
						continue;
						//Alpha-Numeric 이면
						//if ((0x30 <= *(itChar+1) && *(itChar+1) <= 0x39) || (0x41 <= *(itChar+1) && *(itChar+1) <= 0x5A) || (0x61 <= *(itChar+1) && *(itChar+1) <= 0x7A))
						//	EndofSubDelimiter=false;
						//if((0x00 <= *(itChar+1) && *(itChar+1) <= 0x29) || (0x3A <= *(itChar+1) && *(itChar+1) <= 0x40) || (0x5B <= *(itChar+1) && *(itChar+1)<= 0x60) || 0x7B <= *(itChar+1))
						//	EndofSubDelimiter=true;
					
						if(0<iOffset&&iOffset<35)
						{
							++itChar;
							FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
							FieldFormatTemp.iFieldFormatPositioninString=iPositionCount++;
							//FieldFormatTemp.uiFieldFormatIDArrange=FieldFormatID++;
							FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
							//FieldFormatTemp.bSubDelimiterExistent=true;
							FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
							p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
							//FieldFormatTemp.print();
							//printf("iFieldformatPosition = %d\n",FieldFormatTemp.iFieldFormatPositioninString);
							//getchar();
							FieldFormatTemp.reset();
							flag=true;
							break;
							//char 포문 빠져나감
						}

					
				
					
				}

			}
			
			FieldFormatTemp.cContent.vcChars.push_back(*itChar);

			//FieldFormatTemp.print();
			//itFieldFormat->print();
			//getchar();
			//if(EndofSubDelimiter)
			//	break;
			
		}


		//p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
		
		if(!flag )// !EndofSubDelimiter) //Sub Delimiter가 없는 스트링
		{
			FieldFormatTemp.setFieldFormat(&(*itFieldFormat));
			FieldFormatTemp.bRemnantFieldFormat=true;
			//FieldFormatTemp.iFieldFormatPositioninString=0;
			//FieldFormatTemp.print();
			//getchar();
			if(FieldFormatTemp.cContent.uiDirection==DIREC_REQUEST)
				*p_cpSomeRequestMessageSubDelimiterExistent=true;
			else
				*p_cpSomeResponseMessageSubDelimiterExistent=true;
			p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
		}
		
		//getchar();

	}

	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
	{
		deleteFlag=false;
		if(itFieldFormat->cContent.vcChars.size()<3)
		{
			p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			continue;
		}
			for(itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end() ; ++itChar )
			{

				if(*itChar=='(' || *itChar==')' || *itChar=='{' || *itChar=='}' || *itChar=='[' || *itChar==']' || *itChar==',')
				{
					deleteFlag=true;
					break;

				}
					

			}
			if(deleteFlag)
				p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			else
				itFieldFormat++;
	}
	
	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
	{
		if(itFieldFormat->cContent.vcChars.size()==0)
			p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
		else
			itFieldFormat++;
	}

	p_cpFieldFormatList->clFieldFormat.sort(FieldFormat::CompareContentLength());

	puts("splitFieldFormatbySubDelimiter() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
	return p_cpFieldFormatList->clFieldFormat.size();

}
//#################################################################################
int isTextProtocol(SequenceVector* p_cpMessageSequenceVector, bool* bTextProtocol)
{
	vector<Sequence>::iterator			itSequence;
	vector<char>::iterator				itChar;
	double								TextCount=0;
	double								MessageCount=0;
	double								MessageSize=0;
	TimeChecker							cTimeChecker;


	cTimeChecker.startClock();
	puts("isTextProtocol() : start");

	//각메시지에서 80%이상이 printable 문자인지 확인하고 메시지의 80%이상이 printable문자로 이루어진 메시지의 개수가 전체 메시지 개수의 60%이상이면 텍스트 프로토콜
	for (itSequence=p_cpMessageSequenceVector->cvSequenceVector.begin();itSequence!=p_cpMessageSequenceVector->cvSequenceVector.end() ; ++itSequence)
	{
		TextCount=0;
		MessageSize=itSequence->clSequenceContentList.front().vcChars.size();
		for (itChar=itSequence->clSequenceContentList.front().vcChars.begin();itChar!=itSequence->clSequenceContentList.front().vcChars.end() ;++itChar )
		{
			if (((*itChar >=' ') && (*itChar <= '~')) && (*itChar != ';') && (*itChar != '\\') && (*itChar != '"') && (*itChar != '|')&& (*itChar != '<')&& (*itChar != '>'))
				TextCount++;
		}
		
		if(TextCount>= MessageSize*0.8)
		{
			//printf("Text Count = %lf, Message Size = %lf\n",TextCount, MessageSize*0.8);
			MessageCount++;
		}

	}
	
	MessageSize=p_cpMessageSequenceVector->cvSequenceVector.size();
	//printf("Message Count = %lf, Message Size = %lf\n",MessageCount, MessageSize);

	if(MessageCount>=MessageSize*0.6)
		*bTextProtocol=true;
	setTextColorRed();
	setTextTwinkle();
	if(*bTextProtocol)
		printf("It is TextProtocol\n");
	else
		printf("It is BinaryProtocol\n");
	setDefault();
	puts("isTextProtocol() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
}
//####################################################################################################################
int	eliminationValueField(FieldFormatList* p_cpFieldFormatList)
{
	list<FieldFormat>::iterator				itFieldFormat;
	vector<char>::iterator					itChar;
	bool									deleteFlag=false;
	TimeChecker							cTimeChecker;


	cTimeChecker.startClock();
	puts("eliminationValueField() : start");

	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
	{
		deleteFlag=false;
			for (itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end() ; ++itChar )
			{
				if (((*itChar >=' ') && (*itChar <= '~')) && (*itChar != ';') && (*itChar != '\\') && (*itChar != '"') && (*itChar != '|')&& (*itChar != '<')&& (*itChar != '>'))
					continue;
				else
				{
					deleteFlag=true;
					break;
				}
					

			}
			if(deleteFlag)
				p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			else
				itFieldFormat++;
	}

	puts("eliminationValueField() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();

}
//###############################################################################################
int extractRemnantDelimiterFieldFormat(FieldFormatList* p_cpFieldFormatList, FieldFormatList* p_cpDelimiterFieldFormatList, bool* bRemnantDelimiterExistent)
{

	list<FieldFormat>::iterator			itFieldFormat;
	list<FieldFormat>::iterator			itFieldFormat2;
	vector<char>::iterator				itChar;
	vector<Sequence>::iterator		itSequence;
	vector<double>::iterator		itDouble;
	vector<int>::iterator			itInt;
	list<Source>::iterator			itSource;
	list<u_int16_t>::iterator		itOffset;


	vector<int>							viIntTemp;
	vector<vector<int> >				viDelimiterCountVector;	
	vector<double>						vdDelimiterCount;
	vector<double>						vdDelimiterVariance;

	
	vector<double>					vdDoubleTemp;
	vector<double>					vdDelimiterOffsetAverageTemp;
	vector<vector<double> >			vdDelimiterOffsetVarianceofAverage;
	vector<vector<double> >			vdDelimiterFieldFormatOffsetVariance;

	FieldFormatList						FieldFormatListTemp;

	Source								cSourceTemp;
	u_int32_t							iOffset;
	bool								bFirst;
	int									i=0;
	int									iDelimiterCount=0;
	double							dmaxtemp;
	double							dmintemp;

	puts("extractRemnantDelimiterFieldFormat() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();


	FieldFormatListTemp.reset();
	int								iFieldFormatSize=0;
	
	//Remnant String 구성
	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->bRemnantFieldFormat)
		{
			//Sub Delimiter가 존재하지 않는 스트링중에 길이가 4이하인것은 하나의 필드로 판단
			if(itFieldFormat->cContent.vcChars.size()<=4)
			{
				itFieldFormat->bRemnantFieldFormat=false;
				continue;
			}
			
			iFieldFormatSize++;
			//itFieldFormat->print();
			//getchar();
		}

	}

	//Remnant Delimiter 후보 구성
	for (itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end() ;++itFieldFormat )
	{
		if(itFieldFormat->bDelimiterFlag || itFieldFormat->bSubDelimiterFlag)
			continue;
		else
		{
			FieldFormatListTemp.clFieldFormat.push_back(*itFieldFormat);
			//itFieldFormat->print();
		}
			
	}
	

	
	
	
	//모든 Remnant Delimiter 후보 순회
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		//모든 Remnant String 순회
		for(itFieldFormat2=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat2!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat2)
		{
			iDelimiterCount=0;
			if(itFieldFormat2->bRemnantFieldFormat)
			{
				
				for (itChar=itFieldFormat2->cContent.vcChars.begin();itChar!=itFieldFormat2->cContent.vcChars.end();++itChar )
				{
					//printf("%c",*itChar);
					if(itFieldFormat->cContent.vcChars.size()==1)
					{
						if(*itChar==itFieldFormat->cContent.vcChars.front())
						{
							iDelimiterCount++;
						
						}
				
					}
					
					else
					{
						if(*itChar==itFieldFormat->cContent.vcChars.front())
						{
		
							if(*(itChar+1)==itFieldFormat->cContent.vcChars.back())
							{
								iDelimiterCount++;

							}
								
						}
					}

				}// Char
				//printf("\n");
				//getchar();
				viIntTemp.push_back(iDelimiterCount);

			}
			

		}//Remnant String
		viDelimiterCountVector.push_back(viIntTemp);
		viIntTemp.clear();

	}//Remnant Delimiter





















	double temp=0;
	
	double							dDelimiterFieldFormatVariance[iFieldFormatSize];
	double							dDelimiterFieldFormatAverage[iFieldFormatSize];
	//Count 평균의 분산값
	for(int i=0; i<p_cpDelimiterFieldFormatList->clFieldFormat.size(); i++)
	{
		//printf("--------------------------%d------------------------------ \n",i);
		temp=0;
		int j=0;
		int imaxtemp=viDelimiterCountVector[i][j];
		int imintemp=viDelimiterCountVector[i][j];

		for(j=0;j<iFieldFormatSize;j++)
		{
	
			temp+=viDelimiterCountVector[i][j];
			
		}
		dDelimiterFieldFormatAverage[i]=temp/(double)iFieldFormatSize;
		//printf("avg = %lf\n\n",dDelimiterFieldFormatAverage[i]);
		temp=0;
		for(j=0;j<iFieldFormatSize;j++)
		{
			temp+=((viDelimiterCountVector[i][j]-dDelimiterFieldFormatAverage[i]) * (viDelimiterCountVector[i][j]-dDelimiterFieldFormatAverage[i]));
		
		}
		
		dDelimiterFieldFormatVariance[i]=temp/(double)(iFieldFormatSize);
		vdDelimiterCount.clear();




	}


	printf("\n\n");
	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	for(int i=0; i<p_cpDelimiterFieldFormatList->clFieldFormat.size(); i++)
	{
		printf("dDelimiterFieldFormatVariance[%d]=%lf\n",i,dDelimiterFieldFormatVariance[i]);
		itFieldFormat->dRemnantDelimiterFieldFormatCountVariance=dDelimiterFieldFormatVariance[i];
		itFieldFormat++;
	}
	printf("\n\n");


	//모든 Remnant Delimiter 후보에 대하여 Offset 계산
	for(itFieldFormat = p_cpDelimiterFieldFormatList->clFieldFormat.begin(); itFieldFormat != p_cpDelimiterFieldFormatList->clFieldFormat.end(); ++itFieldFormat)
	{
		for(itFieldFormat2=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat2!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat2)
		{
			iOffset=0;
			bFirst=true;
			if(itFieldFormat2->bRemnantFieldFormat)
			{
				do{
					iOffset=itFieldFormat2->cContent.ctnctn(iOffset, &itFieldFormat->cContent);
					if(iOffset!= -1)
					{
						if(bFirst)
						{
							cSourceTemp.reset();
							//clRemnantDelimiterSourceList 에서 SourceID는  FieldFormat ID를 의미
							cSourceTemp.uiSourceID=itFieldFormat2->uiFieldFormatIDArrange;
							cSourceTemp.uiSourceIndex=0;
							cSourceTemp.uilSourceOffsetList.push_back(iOffset);
							itFieldFormat->clRemnantDelimiterSourceList.push_back(cSourceTemp);
							bFirst=false;

						}
						else
						{
							itFieldFormat->clRemnantDelimiterSourceList.back().uilSourceOffsetList.push_back(iOffset);
					
						}


					}

					iOffset++;

				}
				while (iOffset!=0);
			}

			
		}

	}

	//Remnant String마다 Remnant구분자필드 Offset 평균과 분산
	double		dVarianceTemp=0;
	int itemp=0;
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		for(itSource=itFieldFormat->clRemnantDelimiterSourceList.begin();itSource!=itFieldFormat->clRemnantDelimiterSourceList.end();++itSource)
		{
			vdDoubleTemp.clear();
			temp=0;
			dVarianceTemp=0;
			dmintemp=(double)itSource->uilSourceOffsetList.front();
			dmaxtemp=(double)itSource->uilSourceOffsetList.front();

			for(itOffset=itSource->uilSourceOffsetList.begin();itOffset!=itSource->uilSourceOffsetList.end();++itOffset)
			{
			
				vdDoubleTemp.push_back((double)*itOffset);
				
				if(dmintemp>*itOffset)
					dmintemp=(double)*itOffset;
				if(dmaxtemp<*itOffset)
					dmaxtemp=(double)*itOffset;
				
				temp+=(double)*itOffset;
			}

		/*
			각각의 Remnant String에서 구분자 후보의 Offset을 vdDoubleTemp에 저장
			각각의 Remnant String에서 구분자 후보 Offset 평균을 vdDelimiterOffsetAverageTemp에 저장
			각각의 Remnant String에서 구분자 후보 Offset의 분산을 dVarianceTemp에 저장
			각각의 dVarianceTemp를 vdDelimiterVariance에 저장
			각각의 vdDelimiterOffsetAverageTemp를 vdDelimiterOffsetVarianceofAverage에 저장
		*/
			
			temp=temp/(double)vdDoubleTemp.size();
			vdDelimiterOffsetAverageTemp.push_back(temp);

			for(int i=0;i<vdDoubleTemp.size();i++)
			{
				temp+=vdDoubleTemp[i];
			}
			
			temp=temp/(double)vdDoubleTemp.size();
		
			//정규화된 값들에 대한 분산값
			for(int i=0;i<vdDoubleTemp.size();i++)
			{
				dVarianceTemp+=(vdDoubleTemp[i]-temp)*(vdDoubleTemp[i]-temp);
				
			}

			itemp=vdDoubleTemp.size();
			dVarianceTemp=dVarianceTemp/(double)itemp;
			vdDelimiterVariance.push_back(dVarianceTemp);

		
		}
		vdDelimiterFieldFormatOffsetVariance.push_back(vdDelimiterVariance);
		vdDelimiterVariance.clear();
		vdDelimiterOffsetVarianceofAverage.push_back(vdDelimiterOffsetAverageTemp);
		vdDelimiterOffsetAverageTemp.clear();
		
	}


	itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();
	//Remnant String마다 Remnant구분자 필드들의 Offset 평균의 분산
	for(int i=0;i<p_cpDelimiterFieldFormatList->clFieldFormat.size();i++)
	{
		temp=0;
		int j=0;
		if(vdDelimiterOffsetVarianceofAverage[i].size()<=0)
		{

			itFieldFormat++;
			printf("It is not Remnant Delimiter\n\n");
		}
		else
		{
			for(j=0;j<vdDelimiterOffsetVarianceofAverage[i].size();j++)
			{
				temp+=vdDelimiterOffsetVarianceofAverage[i][j];
			}
			temp=temp/vdDelimiterOffsetVarianceofAverage[i].size();

			for(j=0;j<vdDelimiterOffsetVarianceofAverage[i].size();j++)
			{
				vdDelimiterOffsetVarianceofAverage[i][j]=(vdDelimiterOffsetVarianceofAverage[i][j]-temp)*(vdDelimiterOffsetVarianceofAverage[i][j]-temp);
			}
			temp=temp/(double)vdDelimiterOffsetVarianceofAverage[i].size();
			itFieldFormat->dRemnantDelimiterOffsetVarianceofAverage=temp;
			itFieldFormat++;
			printf("Variance of Average = %lf\n\n",temp);

		}


	}
	
	double dCountVariancemintemp=100;
	double dOffsetVariancemaxtemp=100;
	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->dRemnantDelimiterFieldFormatCountVariance==0 && itFieldFormat->dRemnantDelimiterOffsetVarianceofAverage)
			continue;
		if(itFieldFormat->dRemnantDelimiterFieldFormatCountVariance<dCountVariancemintemp)
			dCountVariancemintemp=itFieldFormat->dRemnantDelimiterFieldFormatCountVariance;
		if(itFieldFormat->dRemnantDelimiterOffsetVarianceofAverage>dOffsetVariancemaxtemp)
			dOffsetVariancemaxtemp=itFieldFormat->dRemnantDelimiterOffsetVarianceofAverage;
	}

	for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->dRemnantDelimiterFieldFormatCountVariance==0 || itFieldFormat->dRemnantDelimiterOffsetVarianceofAverage==0)
			continue;
		else
		{
			if(itFieldFormat->dRemnantDelimiterFieldFormatCountVariance<2 && itFieldFormat->dRemnantDelimiterOffsetVarianceofAverage<0.1)
			{
				itFieldFormat->bRemnantDelimiterFlag=true;
				*bRemnantDelimiterExistent=true;
				//itFieldFormat->print();
				//getchar();
				break;
			}

		}

	}
















	puts("extractRemnantDelimiterFieldFormat() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
}
//#############################################################################################################################
int splitFieldFormatbyRemnantDelimiter(FieldFormatList* p_cpFieldFormatList, FieldFormatList*  p_cpRemnantFieldFormatList, FieldFormatList* p_cpDelimiterFieldFormatList)
{
	FieldFormat									FieldFormatTemp;
	FieldFormat									RemnantDelimiter;
	MessageFormatList							RequestMessageFormatListTemp;
	MessageFormatList							ResponseMessageFormatListTemp;
	MessageFormat								MessageFormatTemp;
	list<FieldFormat>::iterator					itFieldFormat;
	list<FieldFormat>::iterator					itFieldFormat2;
	
	list<MessageFormat>::iterator				itMessageFormat;
	
	
	int											RemnantDelimiterCount=0;
	vector<char>::iterator						itChar;
	vector<char>::iterator						itChar2;
	
	puts("splitFieldFormatbyRemnantDelimiter() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();
	p_cpRemnantFieldFormatList->reset();
	FieldFormatTemp.reset();
	RequestMessageFormatListTemp.reset();
	ResponseMessageFormatListTemp.reset();
	RemnantDelimiter.reset();

	//####################
	//Remnant Delimiter 설정
	//####################

		for(itFieldFormat=p_cpDelimiterFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpDelimiterFieldFormatList->clFieldFormat.end();++itFieldFormat)
		{
			if(itFieldFormat->bRemnantDelimiterFlag)
			{
				RemnantDelimiter.setFieldFormat(&(*itFieldFormat));
				break;
			}
		}
	
		
		//#################################################################################
		//Sub Delimiter가 없는 String들을 모아서 RemnantDelimiter로 각각 잘라서 하나의 메시지 포맷으로 구성
		//#################################################################################

		for (itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end() ;++itFieldFormat )
		{
			if(!itFieldFormat->bRemnantFieldFormat)
				continue;

			MessageFormatTemp.reset();
			FieldFormatTemp.reset();
			RemnantDelimiterCount=0;
			for (itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end() ;++itChar )
			{
				
				
				if(RemnantDelimiter.cContent.vcChars.size()==1)
				{
					if(*itChar==RemnantDelimiter.cContent.vcChars.front())
					{
						

						FieldFormatTemp.iFieldFormatPosition=RemnantDelimiterCount++;
						FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
						FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
						FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
						p_cpRemnantFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
						MessageFormatTemp.clFieldFormat.push_back(FieldFormatTemp);
						FieldFormatTemp.reset();
						continue;
					}
			
				}
				
				else
				{
						if(*itChar==RemnantDelimiter.cContent.vcChars.front())
						{
		
							if(*(itChar+1)==RemnantDelimiter.cContent.vcChars.back())
							{
								itChar++;
								FieldFormatTemp.iFieldFormatPosition=RemnantDelimiterCount++;
								FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
								FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
								FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
								p_cpRemnantFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
								MessageFormatTemp.clFieldFormat.push_back(FieldFormatTemp);
								FieldFormatTemp.reset();
								continue;

							}
								
						}
				}	

				FieldFormatTemp.cContent.vcChars.push_back(*itChar);

			}//Remnant String Char
				FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
				FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
				FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
				FieldFormatTemp.iFieldFormatPosition=RemnantDelimiterCount;
				p_cpRemnantFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
				MessageFormatTemp.clFieldFormat.push_back(FieldFormatTemp);
				MessageFormatTemp.iRemnantDelimiterCount=RemnantDelimiterCount;
				if(itFieldFormat->cContent.uiDirection==DIREC_REQUEST)
					RequestMessageFormatListTemp.clMessageFormat.push_back(MessageFormatTemp);
				else
					ResponseMessageFormatListTemp.clMessageFormat.push_back(MessageFormatTemp);

				
				FieldFormatTemp.reset();
			
		}
		
		for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end(); )
		{
			if(itFieldFormat->bRemnantFieldFormat)
			{
				p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			}
			else
				itFieldFormat++;

		}


		//##################################################################################
		//각 메시지 포맷에서 Request/Response로 나눠서 각 Direction에서 RemnantDelimiter 개수가 같은지 확인
		//##################################################################################

		int		RemnantDelimiterCountCheck=0;
		bool	bRequestRemnantDelimiterCount=true;
		bool	bResponseRemnantDelimiterCount=true;
		int		RequestMinRemnantDelimiterCount=10;
		int		ResponseMinRemnantDelimiterCount=10;

		//#########################################
		//Request 헤더라인에서 RemnantDelimiter 개수 확인
		//#########################################

		itMessageFormat=RequestMessageFormatListTemp.clMessageFormat.begin();
		RemnantDelimiterCountCheck=itMessageFormat->iRemnantDelimiterCount;
		itMessageFormat++;
		
		for(;itMessageFormat!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
		{
			if(itMessageFormat->iRemnantDelimiterCount!=RemnantDelimiterCountCheck)
			{
				bRequestRemnantDelimiterCount=false;
				
			}
			if(RequestMinRemnantDelimiterCount>itMessageFormat->iRemnantDelimiterCount)
				RequestMinRemnantDelimiterCount=itMessageFormat->iRemnantDelimiterCount;
			
		}

		//#########################################
		//Response 헤더라인에서 RemnantDelimiter 개수 확인
		//#########################################

		itMessageFormat=ResponseMessageFormatListTemp.clMessageFormat.begin();
		RemnantDelimiterCountCheck=itMessageFormat->iRemnantDelimiterCount;
		itMessageFormat++;
		
		for(;itMessageFormat!=ResponseMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
		{
			if(itMessageFormat->iRemnantDelimiterCount!=RemnantDelimiterCountCheck)
			{
				bResponseRemnantDelimiterCount=false;
				
			}
			if(ResponseMinRemnantDelimiterCount>itMessageFormat->iRemnantDelimiterCount)
				ResponseMinRemnantDelimiterCount=itMessageFormat->iRemnantDelimiterCount;
			
			
		}



	
		
		//####################################################################
		//모든 Request메시지에서 Remnant Delimiter 개수가 같다면 Request 메시지 Hash 구축
		//####################################################################
		if(bRequestRemnantDelimiterCount)
		{
			vector<vector<unsigned int> >		uivRequestVector;
			vector<unsigned int>				uivRequestVectorTemp;
			vector<unsigned int>::iterator		itUiv;
			int									FieldFormatCountinMessageFormat=0;
			
			map<unsigned int, int>				ValueCount;
			map<unsigned int, int>::iterator	itMap;
			ValueCount.clear();
			int									iMessageSize=RequestMessageFormatListTemp.clMessageFormat.size();

			
			uivRequestVector.clear();
			
			uivRequestVectorTemp.clear();
			
			FieldFormatCountinMessageFormat=RequestMessageFormatListTemp.clMessageFormat.front().iRemnantDelimiterCount;

			for(itMessageFormat=RequestMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
			{
				for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
				{
						itFieldFormat->uiFieldFormatHash=itFieldFormat->hashingFieldFormat();
					
				}
			
			}
			
			
			//첫번째, 두번째, 세번째 필드들마다 몇가지의 종류가 나오는지 확인
			for(int i=0;i<FieldFormatCountinMessageFormat+1;i++)
			{
				uivRequestVectorTemp.clear();
				ValueCount.clear();
				for(itMessageFormat=RequestMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
				{
					for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
					{
						if(itFieldFormat->iFieldFormatPosition==i)
						{
							uivRequestVectorTemp.push_back(itFieldFormat->uiFieldFormatHash);
						}
					}
				
				}

				uivRequestVector.push_back(uivRequestVectorTemp);
				sort(uivRequestVectorTemp.begin(),uivRequestVectorTemp.end());
				uivRequestVectorTemp.erase(unique(uivRequestVectorTemp.begin(),uivRequestVectorTemp.end()), uivRequestVectorTemp.end());
				

				
			

				for(itUiv=uivRequestVectorTemp.begin();itUiv!=uivRequestVectorTemp.end();++itUiv)
				{
					ValueCount.insert(make_pair(*itUiv,0));
				}

				for(itMap=ValueCount.begin();itMap!=ValueCount.end();++itMap)
				{
					for(itUiv=uivRequestVector[i].begin();itUiv!=uivRequestVector[i].end();++itUiv)
					{
						if(*itUiv==itMap->first)
						{
							itMap->second=itMap->second+1;

						}
					}
					
				}
				double			dEntropy=0;
				for(itMap=ValueCount.begin();itMap!=ValueCount.end();++itMap)
				{
					dEntropy+=((double)itMap->second / (double)iMessageSize)*logB(((double)itMap->second / (double)iMessageSize),2.0);
				}
				
				dEntropy=-dEntropy;
				
				if(dEntropy<2)
				{
					for(itMessageFormat=RequestMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
					{
						for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
						{
							if(itFieldFormat->iFieldFormatPosition==i)
							{
								p_cpFieldFormatList->clFieldFormat.push_back(*itFieldFormat);
							}
						}
					
					}

				}
				//printf("[%d] FieldFormat Entropy = %lf\n",i,dEntropy);
				//getchar();


			}

			uivRequestVector.clear();

			uivRequestVectorTemp.clear();
			


			//################################################################
			//같은 해쉬값을 가지면서 다른 Content값을 가지는 필드포맷이 있는지 검사
			//################################################################
			/*
			for(itMessageFormat=RequestMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
			{
				itMessageFormat2=itMessageFormat++;
				itMessageFormat--;
				for(;itMessageFormat2!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat2)
				{
					itFieldFormat=itMessageFormat->clFieldFormat.begin();
					itFieldFormat2=itMessageFormat2->clFieldFormat.begin();
					itFieldFormat++;
					itFieldFormat2++;
					itFieldFormat++;
					itFieldFormat2++;
					if(itFieldFormat->uiFieldFormatHash==itFieldFormat2->uiFieldFormatHash)
					{
						if(!(itFieldFormat->CompareContentValue(&(*itFieldFormat2))))
						{
							itFieldFormat->print();
						itFieldFormat->print();
						printf("\n\n\n\n\n\n\n\n\n\n\n\n");
						getchar();

						}
					
						
					}
				
				}
			}
		*/

		}//Request 메시지에서 Remnant Delimiter 개수가 모두 같을경우
		else
		{
			for(itMessageFormat=RequestMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=RequestMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
			{
				FieldFormatTemp.reset();
				int i=0;
				for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
				{

					if(itMessageFormat->iRemnantDelimiterCount==RequestMinRemnantDelimiterCount)
					{
						p_cpFieldFormatList->clFieldFormat.push_back(*itFieldFormat);
					}
					else
					{
						if(i<RequestMinRemnantDelimiterCount)
							p_cpFieldFormatList->clFieldFormat.push_back(*itFieldFormat);
						else
						{
							FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
							FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
							FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
							for(itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end();++itChar)
							{
								FieldFormatTemp.cContent.vcChars.push_back(*itChar);
							}
							if(i!=itMessageFormat->iRemnantDelimiterCount)
							{
								if(RemnantDelimiter.cContent.vcChars.size()==1)
								{
									FieldFormatTemp.cContent.vcChars.push_back(RemnantDelimiter.cContent.vcChars.front());
								}

								else
								{
									
									FieldFormatTemp.cContent.vcChars.push_back(RemnantDelimiter.cContent.vcChars.front());
									FieldFormatTemp.cContent.vcChars.push_back(RemnantDelimiter.cContent.vcChars.back());
									
								}

							}
							

						}
						
						
					
					}
					
					i++;
				}//필드포맷 포문

		

				p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
				FieldFormatTemp.reset();

			}

		}
		
		//################################################
		//Response 메시지에서 Remnant Delimiter의 개수가 같을 경우
		//################################################
		if(bResponseRemnantDelimiterCount)
		{
			vector<vector<unsigned int> >		uivResponseVector;
			vector<unsigned int>				uivResponseVectorTemp;
			vector<unsigned int>::iterator		itUiv;
			int									FieldFormatCountinMessageFormat=0;
			
			map<unsigned int, int>				ValueCount;
			map<unsigned int, int>::iterator	itMap;
			ValueCount.clear();
			int									iMessageSize=ResponseMessageFormatListTemp.clMessageFormat.size();

			
			uivResponseVector.clear();
			
			uivResponseVectorTemp.clear();
			
			FieldFormatCountinMessageFormat=ResponseMessageFormatListTemp.clMessageFormat.front().iRemnantDelimiterCount;

			for(itMessageFormat=ResponseMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=ResponseMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
			{
				for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
				{
						itFieldFormat->uiFieldFormatHash=itFieldFormat->hashingFieldFormat();
					
				}
			
			}
			
			
			//첫번째, 두번째, 세번째 필드들마다 몇가지의 종류가 나오는지 확인
			for(int i=0;i<FieldFormatCountinMessageFormat+1;i++)
			{
				uivResponseVectorTemp.clear();
				ValueCount.clear();
				for(itMessageFormat=ResponseMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=ResponseMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
				{
					for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
					{
						if(itFieldFormat->iFieldFormatPosition==i)
						{
							uivResponseVectorTemp.push_back(itFieldFormat->uiFieldFormatHash);
						}
					}
				
				}

				uivResponseVector.push_back(uivResponseVectorTemp);
				sort(uivResponseVectorTemp.begin(),uivResponseVectorTemp.end());
				uivResponseVectorTemp.erase(unique(uivResponseVectorTemp.begin(),uivResponseVectorTemp.end()), uivResponseVectorTemp.end());
				

				
			

				for(itUiv=uivResponseVectorTemp.begin();itUiv!=uivResponseVectorTemp.end();++itUiv)
				{
					ValueCount.insert(make_pair(*itUiv,0));
				}

				for(itMap=ValueCount.begin();itMap!=ValueCount.end();++itMap)
				{
					for(itUiv=uivResponseVector[i].begin();itUiv!=uivResponseVector[i].end();++itUiv)
					{
						if(*itUiv==itMap->first)
						{
							itMap->second=itMap->second+1;

						}
					}
					
				}
				double			dEntropy=0;
				for(itMap=ValueCount.begin();itMap!=ValueCount.end();++itMap)
				{
					dEntropy+=((double)itMap->second / (double)iMessageSize)*logB(((double)itMap->second / (double)iMessageSize),2.0);
				}
				
				dEntropy=-dEntropy;
				
				if(dEntropy<2)
				{
					for(itMessageFormat=ResponseMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=ResponseMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
					{
						for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
						{
							if(itFieldFormat->iFieldFormatPosition==i)
							{
								p_cpFieldFormatList->clFieldFormat.push_back(*itFieldFormat);
							}
						}
					
					}

				}
				//printf("[%d] FieldFormat Entropy = %lf\n",i,dEntropy);
				//getchar();


			}

			uivResponseVector.clear();

			uivResponseVectorTemp.clear();

		}
		//#########################################################################################
		//Response 메시지에서 Remnant Delimiter의 개수가 다를경우 개수의 최솟값 까지만 구분자로 판단하고 잘라서 필드로 추출
		//#########################################################################################
		else
		{
			for(itMessageFormat=ResponseMessageFormatListTemp.clMessageFormat.begin();itMessageFormat!=ResponseMessageFormatListTemp.clMessageFormat.end();++itMessageFormat)
			{
				//itMessageFormat->print();
				//getchar();
				FieldFormatTemp.reset();
				int i=0;
				for(itFieldFormat=itMessageFormat->clFieldFormat.begin();itFieldFormat!=itMessageFormat->clFieldFormat.end();++itFieldFormat)
				{

					if(itMessageFormat->iRemnantDelimiterCount==ResponseMinRemnantDelimiterCount)
					{
						p_cpFieldFormatList->clFieldFormat.push_back(*itFieldFormat);
					}
					else
					{
						if(i<ResponseMinRemnantDelimiterCount)
							p_cpFieldFormatList->clFieldFormat.push_back(*itFieldFormat);
						else
						{
							FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
							FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
							FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());
							for(itChar=itFieldFormat->cContent.vcChars.begin();itChar!=itFieldFormat->cContent.vcChars.end();++itChar)
							{
								FieldFormatTemp.cContent.vcChars.push_back(*itChar);
							}
							if(i!=itMessageFormat->iRemnantDelimiterCount)
							{
								if(RemnantDelimiter.cContent.vcChars.size()==1)
								{
									FieldFormatTemp.cContent.vcChars.push_back(RemnantDelimiter.cContent.vcChars.front());
								}

								else
								{
									
									FieldFormatTemp.cContent.vcChars.push_back(RemnantDelimiter.cContent.vcChars.front());
									FieldFormatTemp.cContent.vcChars.push_back(RemnantDelimiter.cContent.vcChars.back());
									
								}

							}
							

						}
						
						
					
					}
					
					i++;
				}//필드포맷 포문

				//Check!!!
				/*
				if(RemnantDelimiter.cContent.vcChars.size()==1)
				{
					FieldFormatTemp.cContent.vcChars.pop_back();
				}

				else
				{
					FieldFormatTemp.cContent.vcChars.pop_back();
					FieldFormatTemp.cContent.vcChars.pop_back();
				}
				*/

				p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
				FieldFormatTemp.reset();

			}
			


		}
		
		





	puts("splitFieldFormatbyRemnantDelimiter() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
}
//###################################################################
int extractRemnantFieldFormat(FieldFormatList* p_cpFieldFormatList)
{
	FieldFormat								FieldFormatTemp;
	list<FieldFormat>::iterator				itFieldFormat;
	vector<char>::iterator					itChar;
	bool									bRequestNotRemnantFieldFormatPositionSame=true;
	bool									bResponseNotRemnantFieldFormatPositionSame=true;
	bool									bRequestNotRemnantFieldFormatContentSize=true;
	bool									bResponseNotRemnantFieldFormatContentSize=true;
	int										iRequestFieldFormatContentSize=0;
	int										iResponseFieldFormatContentSize=0;
	int										iRequestNotRemnantFieldFormatPosition=-1;
	int										iResponseNotRemnantFieldFormatPosition=-1;

	puts("extractRemnantFieldFormat() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();

	//##############################################################
	//Sub Delimiter가 없는 애들은 bRemnantFieldFormat
	//##############################################################
	deDuplicationFieldFormat(p_cpFieldFormatList);

	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(!itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.uiDirection==DIREC_REQUEST)
		{

			iRequestFieldFormatContentSize=itFieldFormat->cContent.vcChars.size();
			iRequestNotRemnantFieldFormatPosition=itFieldFormat->iFieldFormatPositioninString;
			break;
	
		}
	}

	
	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(!itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.uiDirection==DIREC_RESPONSE)
		{

			iResponseFieldFormatContentSize=itFieldFormat->cContent.vcChars.size();
			iResponseNotRemnantFieldFormatPosition=itFieldFormat->iFieldFormatPositioninString;
			break;

		}
	}

	

	//###################################################################################
	//RemnantFieldFormat이 아니고 Request인 필드들의 사이즈가 모두 같은지 검사
	//RemnantFieldFormat이 아니고 Response인 필드들이 Sub Delimiter로 자를때 필드 인덱스가 모두 같은지 검사
	//###################################################################################
	
	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(!itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.uiDirection==DIREC_REQUEST)
		{
			
			
			if(itFieldFormat->cContent.vcChars.size()!=iRequestFieldFormatContentSize)
			{
				bRequestNotRemnantFieldFormatContentSize=false;
				//itFieldFormat->print();
				//getchar();
			}
				
			if(itFieldFormat->iFieldFormatPositioninString != iRequestNotRemnantFieldFormatPosition)
				bRequestNotRemnantFieldFormatPositionSame=false;


			//printf("Request iFieldFormatPositioninString = %d\n",itFieldFormat->iFieldFormatPositioninString);
		}
		

	}

	//###################################################################################
	//RemnantFieldFormat이 아니고 Response인 필드들의 사이즈가 모두 같은지 검사
	//RemnantFieldFormat이 아니고 Response인 필드들이 Sub Delimiter로 자를때 필드 인덱스가 모두 같은지 검사
	//###################################################################################

	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(!itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.uiDirection==DIREC_RESPONSE)
		{
			
			//printf("Response Not Remnant Field Format size = %d\n",itFieldFormat->cContent.vcChars.size());
			if(itFieldFormat->cContent.vcChars.size()!=iResponseFieldFormatContentSize)
			{
				bResponseNotRemnantFieldFormatContentSize=false;
				//itFieldFormat->print();
				//getchar();
			}
				
			if(itFieldFormat->iFieldFormatPositioninString != iResponseNotRemnantFieldFormatPosition)
				bResponseNotRemnantFieldFormatPositionSame=false;
			/*
			printf("Response iFieldFormatPositioninString = %d\n",itFieldFormat->iFieldFormatPositioninString);
			if(itFieldFormat->iFieldFormatPositioninString == -1)
				itFieldFormat->print();
				*/
		}

	}

	//##########################################################
	//Remnant FieldFormat 이면서 길이가 4이하인 애들은 필드로 추출
	//##########################################################

	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
	{
		if(itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.vcChars.size()<=4)
		{
			//itFieldFormat->print();
			//getchar();
			FieldFormatTemp.reset();
			
				itChar=itFieldFormat->cContent.vcChars.begin();
				for(int i=0;i<itFieldFormat->cContent.vcChars.size();i++)
				{
					FieldFormatTemp.cContent.vcChars.push_back(*itChar);
					itChar++;

				}
				
				FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
				FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
				FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());

				//printf("sfghdfghdfghdfgh");
				//itFieldFormat->print();
				//FieldFormatTemp.print();
				//getchar();
				itFieldFormat->bDeleteFlag=true;
				p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
			
		}
	}



		for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
		{
			if(itFieldFormat->bDeleteFlag)
			{
				//itFieldFormat->print();
				p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
				
			}
			else
				itFieldFormat++;
		}


	//######################################################################################################################
	//Remnant FieldFormat 아닌 애들이 Sub Delimiter로 잘랐을때 필드인덱스가 모두 같고 길이가 모두 같으면 Remnant FieldFormat을 해당길이까지 잘라서 추출
	//######################################################################################################################
	if(bRequestNotRemnantFieldFormatPositionSame && bRequestNotRemnantFieldFormatContentSize)
	{
		for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
		{
			FieldFormatTemp.reset();
			if(itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.uiDirection==DIREC_REQUEST)
			{
				itChar=itFieldFormat->cContent.vcChars.begin();
				for(int i=0;i<iRequestFieldFormatContentSize;i++)
				{
					FieldFormatTemp.cContent.vcChars.push_back(*itChar);
					itChar++;

				}
				
				FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
				FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
				FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());

				//printf("sfghdfghdfghdfgh");
				//itFieldFormat->print();
				//FieldFormatTemp.print();
				//getchar();
				itFieldFormat->bDeleteFlag=true;
				p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
			}
		
		}

	}

	//######################################################################################################################
	//Remnant FieldFormat 아닌 애들이 Sub Delimiter로 잘랐을때 필드인덱스가 모두 같고 길이가 모두 같으면 Remnant FieldFormat을 해당길이까지 잘라서 추출
	//######################################################################################################################
	if(bResponseNotRemnantFieldFormatPositionSame && bResponseNotRemnantFieldFormatContentSize)
	{
		for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();++itFieldFormat)
		{
			FieldFormatTemp.reset();
			if(itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.uiDirection==DIREC_RESPONSE)
			{
				itChar=itFieldFormat->cContent.vcChars.begin();
				for(int i=0;i<iResponseFieldFormatContentSize;i++)
				{
					FieldFormatTemp.cContent.vcChars.push_back(*itChar);
					itChar++;

				}
				
				FieldFormatTemp.cContent.uiDirection=itFieldFormat->cContent.uiDirection;
				FieldFormatTemp.cContent.cContentHeader.uiProtocol=itFieldFormat->cContent.cContentHeader.uiProtocol;
				FieldFormatTemp.uivIdentifiedMessageIdList.assign(itFieldFormat->uivIdentifiedMessageIdList.begin(), itFieldFormat->uivIdentifiedMessageIdList.end());

				//printf("sfghdfghdfghdfgh");
				//itFieldFormat->print();
				//FieldFormatTemp.print();
				//getchar();
				itFieldFormat->bDeleteFlag=true;
				p_cpFieldFormatList->clFieldFormat.push_back(FieldFormatTemp);
			}
		
		}

	}

	for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
	{
		if(itFieldFormat->bDeleteFlag)
		{
			//itFieldFormat->print();
			p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			
		}
		else
			itFieldFormat++;
	}
	//####################################################################################
	//Remnant FieldFormat이 아닌 애들이 길이가 모두 같다면 그 길이와 길이가 다른 Remnant FieldFormat 제거
	//####################################################################################
	if(bRequestNotRemnantFieldFormatContentSize)
	{
		for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
		{
			if(itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.vcChars.size()!=iRequestFieldFormatContentSize && itFieldFormat->cContent.uiDirection==DIREC_REQUEST)
			{
				//itFieldFormat->print();
				//getchar();
				p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			}
			else
				itFieldFormat++;

		}
	}
	//####################################################################################
	//Remnant FieldFormat이 아닌 애들이 길이가 모두 같다면 그 길이와 길이가 다른 Remnant FieldFormat 제거
	//####################################################################################
	if(bResponseNotRemnantFieldFormatContentSize)
	{
		for(itFieldFormat=p_cpFieldFormatList->clFieldFormat.begin();itFieldFormat!=p_cpFieldFormatList->clFieldFormat.end();)
		{
			if(itFieldFormat->bRemnantFieldFormat && itFieldFormat->cContent.vcChars.size()!=iResponseFieldFormatContentSize && itFieldFormat->cContent.uiDirection==DIREC_RESPONSE)
			{
				//itFieldFormat->print();
				//getchar();
				p_cpFieldFormatList->clFieldFormat.erase(itFieldFormat++);
			}
			else
				itFieldFormat++;

		}
	}

	



	puts("extractRemnantFieldFormat() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();


	return p_cpFieldFormatList->clFieldFormat.size();
}
//################################################################################################################################
int deleteDataMessageSequence(SequenceVector* p_cpMessageSequenceVector)
{
	vector<Sequence>::iterator					itSequence;
	double								dMessageSequnceLengthAverage=0;
	double								temp=0;

	puts("deleteDataMessageSequence() : start");

	TimeChecker						cTimeChecker;
	cTimeChecker.startClock();
	for(itSequence=p_cpMessageSequenceVector->cvSequenceVector.begin();itSequence!=p_cpMessageSequenceVector->cvSequenceVector.end();++itSequence)
	{
		dMessageSequnceLengthAverage+=itSequence->clSequenceContentList.front().vcChars.size();
		//printf("MessageSequence Length = %d\n",itSequence->clSequenceContentList.front().vcChars.size());
	}
	dMessageSequnceLengthAverage=dMessageSequnceLengthAverage/(double)p_cpMessageSequenceVector->cvSequenceVector.size();
	//printf("MessageSequence Length Average = %lf\n",dMessageSequnceLengthAverage);
	for(itSequence=p_cpMessageSequenceVector->cvSequenceVector.begin();itSequence!=p_cpMessageSequenceVector->cvSequenceVector.end();)
	{
		temp=(itSequence->clSequenceContentList.front().vcChars.size()-dMessageSequnceLengthAverage);
		if(temp<0)
			temp=-temp;
		if(temp>=900)
			p_cpMessageSequenceVector->cvSequenceVector.erase(itSequence++);
		else
			itSequence++;

	}
	/*
	for(itSequence=p_cpMessageSequenceVector->cvSequenceVector.begin();itSequence!=p_cpMessageSequenceVector->cvSequenceVector.end();++itSequence)
	{

		printf("MessageSequence Length = %d\n",itSequence->clSequenceContentList.front().vcChars.size());
	}
	*/
	puts("deleteDataMessageSequence() : end");
	cTimeChecker.endClock();
	cTimeChecker.print();
	return p_cpMessageSequenceVector->cvSequenceVector.size();

}
//###############################################################################################
void showInteractiveMenu(FlowHash* p_cpFlowHash, SequenceVector* p_cpFlowSequenceVector, SequenceVector* p_cpMessageSequenceVector, SequenceVector* p_cpFieldSequenceVector, FieldFormatList* p_cpFieldFormatList, MessageFormatList* p_cpMessageFormatList, FlowFormatList* p_cpFlowFormatList)
{
	vector<State>::iterator				itState;
	list<FieldFormat>::iterator			itFieldFormat;
	list<MessageFormat>::iterator		itMessageFormat;
	list<FlowFormat>::iterator			itFlowFormat;
	int iRequestSFV, iResponseSFV, iRequestDFV, iResponseDFV;
	int iRequestFormat, iResponseFormat;
	double	dTotalCoverage;
	u_int32_t uiTotalCoverMsgCount;

	int	input_a;
	int input_b; //MessageFormatID
	int input_c; //FlowFormatID

	system("stty erase ^H");

	setTextColorRed();
	setTextTwinkle();
	printf("Press return key to menu...\n");
	setDefault();
	getchar();

	do
	{
		input_a = 0;
		input_b = 0;
		input_c = 0;

		system("clear");
		printf("---------------------------------------------------------\n");
		setTextColorRed();
		printf("Protocol Analyzer Menu\n");
		printf("1 : Show Traffic Information\n");
		printf("2 : Show Field Format Information\n");
		printf("3 : Show Message Format Information\n");
		printf("4 : Show Flow Format Information\n");
		//printf("5 : Show FSM Information\n");
		//printf("6 : Show Representative Message Format Information\n");
		printf("7 : Exit\n");
		setDefault();
		printf("---------------------------------------------------------\n");
		printf("input : ");

		scanf("%d", &input_a);

		//show threshold

		switch(input_a)
		{
			case 1:
				setTextColorBlue();
				printf("Traffic Summary.\n");
				setDefault();
				printf("flow : %7llu pkt : %7llu byte : %7llu\r\n",g_cTotalVolume.flow,g_cTotalVolume.pkt,g_cTotalVolume.byte);
				printf("message : %7d(Request : %d, Response : %d)\r\n",p_cpMessageSequenceVector->cvSequenceVector.size(),g_iDIREC_REQUESTMsgCount,g_iDIREC_RESPONSEMsgCount);
				printf("\r\n");

				printf("Press return key to continue...\n");
				getchar();
				getchar();
				break;
			case 2:
				setTextColorBlue();
				printf("Field Format Summary.\n");
				setDefault();
				setTextColorGreen();
				printf("Field Format : %3d\r\n", p_cpFieldFormatList->clFieldFormat.size());
				setDefault();
				iRequestSFV = iResponseSFV = iRequestDFV = iResponseDFV = 0;
				for(itFieldFormat = p_cpFieldFormatList->clFieldFormat.begin() ; itFieldFormat != p_cpFieldFormatList->clFieldFormat.end() ; ++itFieldFormat)
				{
					if(itFieldFormat->cvAdditionalContentVector.size() == 0)
					{
						if(itFieldFormat->cContent.uiDirection == DIREC_REQUEST)
							iRequestSFV++;
						else
							iResponseSFV++;
					}
					else
					{
						if(itFieldFormat->cContent.uiDirection == DIREC_REQUEST)
							iRequestDFV++;
						else
							iResponseDFV++;
					}
				}
				printf("      Request : %3d(SF(v) : %3d, DF(v) : %3d)\r\n", iRequestSFV + iRequestDFV, iRequestSFV, iRequestDFV);
				printf("      Response : %3d(SF(v) : %3d, DF(v) : %3d)\r\n", iResponseSFV + iResponseDFV, iResponseSFV, iResponseDFV);


				p_cpFieldFormatList->printInMenu();
				printf("\r\n");

				printf("Press return key to continue...\n");
				getchar();
				getchar();
				break;
			case 3:
				showMessageFormatMenu(p_cpFlowHash, p_cpFlowSequenceVector, p_cpMessageSequenceVector, p_cpFieldSequenceVector, p_cpFieldFormatList, p_cpMessageFormatList, p_cpFlowFormatList);
				break;
			case 4:
				setTextColorBlue();
				printf("Flow Format Summary.\n");
				setDefault();
				setTextColorGreen();
				printf("Flow Format : %3d\r\n", p_cpFlowFormatList->clFlowFormat.size());
				setDefault();
				setTextColorRed();
				printf("      Completeness : flow = %.2f%%(%llu/%llu) msg = %.2f%%(%llu/%d) pkt = %.2f%%(%llu/%llu) byte = %.2f%%(%llu/%llu)\r\n",
					(float)g_cFlowVolume.flow*100/g_cTotalVolume.flow, g_cFlowVolume.flow, g_cTotalVolume.flow,
					(float)g_cFlowVolume.msg*100/p_cpMessageSequenceVector->cvSequenceVector.size(), g_cFlowVolume.msg, p_cpMessageSequenceVector->cvSequenceVector.size(),
					(float)g_cFlowVolume.pkt*100/g_cTotalVolume.pkt, g_cFlowVolume.pkt, g_cTotalVolume.pkt,
					(float)g_cFlowVolume.byte*100/g_cTotalVolume.byte, g_cFlowVolume.byte, g_cTotalVolume.byte);
				printf("\r\n");
				setDefault();


				p_cpFlowFormatList->printInMenu(g_cTotalVolume.flow);
/*				printf("Show [n]th Flow Format, input n : ");
				scanf("%d", &input_b);
				while(input_b >= p_cpFlowFormatList->clFlowFormat.size())
				{
					printf("Warning : invalid input\n");
					printf("Flow Format ID arrange is [0 ~ %d]\n",p_cpFlowFormatList->clFlowFormat.size()-1);
					printf("Show [n]th Flow Format, input n : ");
					scanf("%d", &input_b);
				}

				itFlowFormat = p_cpFlowFormatList->clFlowFormat.begin();
				std::advance(itFlowFormat,input_b);
				itFlowFormat->printInMenu();
*/
				printf("\r\n");
				printf("Press return key to continue...\n");
				getchar();
				getchar();
				break;
			//case 5:
			//	showFSMMenu(p_cpFlowHash, p_cpFlowSequenceVector, p_cpMessageSequenceVector, p_cpFieldSequenceVector, p_cpFieldFormatList, p_cpMessageFormatList, p_cpFlowFormatList, p_cpFSMManager);
			//	break;
			//case 6:
			//	showMessageFormatMenu(p_cpFlowHash, p_cpFlowSequenceVector, p_cpMessageSequenceVector, p_cpFieldSequenceVector, p_cpFieldFormatList, p_cpRepreMessageFormatList, p_cpFlowFormatList);
			//	break;
			case 7:
				return;
			default :
				while(getchar()!='\n');
				break;
		}
	}
	while (1);
}
//###############################################################################################################
void showMessageFormatMenu(FlowHash* p_cpFlowHash, SequenceVector* p_cpFlowSequenceVector, SequenceVector* p_cpMessageSequenceVector, SequenceVector* p_cpFieldSequenceVector, FieldFormatList* p_cpFieldFormatList, MessageFormatList* p_cpMessageFormatList, FlowFormatList* p_cpFlowFormatList)
{
	list<MessageFormat>::iterator		itMessageFormat;

	int iRequestFormat, iResponseFormat;
	double	dTotalCoverage;
	u_int32_t uiTotalCoverMsgCount;

	int input_a;
	int input_b;

	iRequestFormat = iResponseFormat = 0;
	dTotalCoverage = 0;
	uiTotalCoverMsgCount = 0;
	for(itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin() ; itMessageFormat != p_cpMessageFormatList->clMessageFormat.end() ; ++itMessageFormat)
	{
		if(itMessageFormat->clFieldFormat.front().cContent.uiDirection == DIREC_REQUEST)
			iRequestFormat++;
		else
			iResponseFormat++;

		dTotalCoverage += itMessageFormat->dCoverage;
		uiTotalCoverMsgCount += itMessageFormat->uiCoverdMessage;
	}

	do
	{
		input_a=0;

		system("clear");
		setTextColorBlue();
		printf("Message Format Summary.\n");
		setDefault();
		setTextColorGreen();
		printf("Message Format : %3d\r\n", p_cpMessageFormatList->clMessageFormat.size());
		setDefault();
		printf("      Request : %3d, Response : %3d\r\n",iRequestFormat,iResponseFormat);
		setTextColorRed();
		printf("      Completeness : msg = %.2lf%%(%d/%d)\r\n",
			(double)dTotalCoverage*100, uiTotalCoverMsgCount, p_cpMessageSequenceVector->cvSequenceVector.size());
		setDefault();
		p_cpMessageFormatList->printInMenu();
		printf("---------------------------------------------------------\n");
		setTextColorRed();
		printf("Message Format Info Menu : \n");
		printf("1 : Show [n]th Message Format\n");
		printf("2 : Go to Upper Menu\n");
		setDefault();
		printf("---------------------------------------------------------\n");
		printf("input : ");
		scanf("%d", &input_a);
		switch(input_a)
		{
			case 1:
				if (p_cpMessageFormatList->clMessageFormat.size())
				{
					printf("Show [n]th Message Format, input n : ");
					scanf("%d", &input_b);
					while(input_b >= p_cpMessageFormatList->clMessageFormat.size())
					{
						printf("Warning : invalid input\n");
						printf("Message Format ID arrange is [0 ~ %d]\n",p_cpMessageFormatList->clMessageFormat.size()-1);
						printf("Show [n]th Message Format, input n : ");
						scanf("%d", &input_b);
					}

					itMessageFormat = p_cpMessageFormatList->clMessageFormat.begin();
					std::advance(itMessageFormat,input_b);
					itMessageFormat->printInMenu();

					printf("\r\n");
					printf("Press return key to continue...\n");
					getchar();
					getchar();
				}
				break;
			case 2:
				return;
			default:
				printf("Warning : invalid input\n");
				break;
		}
	}
	while (1);
}
