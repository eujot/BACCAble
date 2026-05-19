/*
 * processingMessage0x000002FA.c
 *
 *  Created on: May 3, 2025
 *      Author: GauchoHP
 */

#include "debug.h"
#include "processingMessage0x000002FA.h"

void processingMessage0x000002FA(){
	// Button is pressed on left area of the wheel
	// These Buttons are detected only if the main panel of the car is on.

	#if defined(C2baccable) || defined(C1baccable)
		if(HAS_buttonPressRequested){ //Has button press was requested
			HAS_buttonPressRequested--;
			if(rx_msg_data[0]==0x10){ //if no button was pressed on cruise control pad
				rx_msg_data[1] = rx_msg_data[1] | 0x10; //simulate HAS button presses
				rx_msg_data[2] = calculateCRC(rx_msg_data,rx_msg_header.DLC); //CRC
				can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //send message to simulate RES button press

			}
		}
	#endif

	#if defined(C1baccable)




	  	//function ACC Virtual Pad

		if(function_acc_virtual_pad_enabled==1){
			switch (rx_msg_data[0]){
				case 0x12: //CC on
					newWheelPressedButtonID=0x12;
					memcpy(ACC_msg_data, &rx_msg_data, rx_msg_header.DLC);
					ACC_msg_data[0] = 0x11; //ACC On
					ACC_msg_data[1] = (ACC_msg_data[1] & 0xF0) | (((ACC_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
					ACC_msg_data[2] = calculateCRC(ACC_msg_data,rx_msg_header.DLC); //update checksum
					can_tx(&ACC_msg_header, ACC_msg_data); //send msg
					onboardLed_blue_on();
					break;
				case 0x90: //RES pressed
					if (newWheelPressedButtonID==0x10 && ACC_engaged) ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=1; //if begin to press button RES and ACC is Engaged, set ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED
					newWheelPressedButtonID=0x90; //store the new RES button status (pressed)
					if (ACC_engaged && ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED){
						//simulate the distance button press
						memcpy(ACC_msg_data, &rx_msg_data, rx_msg_header.DLC);
						ACC_msg_data[0] = 0x50; //ACC distance change
						ACC_msg_data[1] = (ACC_msg_data[1] & 0xF0) | (((ACC_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
						ACC_msg_data[2] = calculateCRC(ACC_msg_data,rx_msg_header.DLC); //update checksum
						can_tx(&ACC_msg_header, ACC_msg_data); //send msg
						onboardLed_blue_on();
					}
					break;
				case 0x10: //button released
					newWheelPressedButtonID=0x10; //button released (I use another variable to distinguish from the one used in show params function
					ACC_WAS_ENGAGED_WHEN_RES_WAS_PRESSED=0;
					break;
				default:
			}
		}

		if(function_acc_autostart){
			if(ACC_engaged){
				if(carSteadyCounter==200 && brakeIntervention_ACC_ESC_ASR){ //if car is steady and brake is pressed by ACC
					if(rx_msg_data[0]==0x10){ //if no button was pressed on cruise control pad
						if (currentTime-lastSentAutostartMsg>500){ //once each 1,5 seconds
							rx_msg_data[0] = 0x90; //Res button press

							if(function_acc_autostart==2){
								rx_msg_data[0] = 0x08; //ACC gently up button press
							}
							rx_msg_data[1] = (rx_msg_data[1] & 0xF0) | (((rx_msg_data[1] & 0x0F) + 1) % 16); //increase the counter
							rx_msg_data[2]=calculateCRC(rx_msg_data,rx_msg_header.DLC); //update checksum

							can_tx((CAN_TxHeaderTypeDef *)&rx_msg_header, rx_msg_data); //send message to simulate RES button press
							rx_msg_data[0]=0x10; //restore value 10 to avoid unwanted behaviours with subsequent pieces of code
							//increase a counter
							AutostartMsgCounter++;
							if (AutostartMsgCounter>= 5){ //we are simulating a 100msec button press event
								AutostartMsgCounter=0;
								lastSentAutostartMsg=currentTime;
							}
						}
					}
				}
			}
		}

		if(cruiseControlDisabled && ACC_Disabled){ //if we are allowed to press buttons, use them in baccable menu
			switch(rx_msg_data[0]){
				case 0x18://if cruise control speed reduction button was pressed, user wants to see next page
					if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
						wheelPressedButtonID=0x18; //avoid to return here
						if(commandsMenuEnabled){
							switch(dashboard_menu_indent_level){
								case 0: //main menu
									main_dashboardPageIndex+= 1; //set next page

									if(function_read_faults_enabled==0){
										if(main_dashboardPageIndex==2) main_dashboardPageIndex++;
									}


									if(function_clear_faults_enabled==0){
										if(main_dashboardPageIndex==3) main_dashboardPageIndex++;
									}

									if(function_dyno_mode_master_enabled==0){
										if(main_dashboardPageIndex==5) main_dashboardPageIndex++;
									}

									if(function_esc_tc_customizator_enabled==0){
										if(main_dashboardPageIndex==6) main_dashboardPageIndex++;
									}
									if(function_front_brake_forcer_master==0){
										if(main_dashboardPageIndex==7) main_dashboardPageIndex++;
									}

									if(function_4wd_disabler_enabled==0){
										if(main_dashboardPageIndex==8) main_dashboardPageIndex++;
									}

									if(HAS_function_enabled==0){
										if(main_dashboardPageIndex==11) main_dashboardPageIndex++;
									}

									if(QV_exhaust_flap_function_enabled==0){
										if(main_dashboardPageIndex==12) main_dashboardPageIndex++;
									}

									if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=0; // make a rotative menu
									//onboardLed_blue_on();
									sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
									break;
								case 1:
									if(main_dashboardPageIndex==1){ //we are in show params submenu
										dashboardPageIndex += 1; //set next page
										if(function_is_diesel_enabled==1){
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // make a rotative menu
										}else{
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // make a rotative menu
										}

										dashboardPageIndex=getNextVisibleParam(dashboardPageIndex);
										//onboardLed_blue_on();
										dashboardParamCouple[0]=NAN;//zeroize params
										dashboardParamCouple[1]=NAN;//zeroize params

										sendDashboardPageToSlaveBaccable(); //send dashboard page to BH
									}
									if(main_dashboardPageIndex==9){ //we are in setup menu
										setup_move_page(1);
										sendSetupDashboardPageToSlaveBaccable(); //send
									}
									if(main_dashboardPageIndex==10){ //we are in params setup menu
										params_setup_dashboardPageIndex+=1;//set next page
										if(params_setup_dashboardPageIndex>total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=0; // make a rotative menu
										sendParamsSetupDashboardPageToSlaveBaccable(); //send
									}
									break;
								default:
									break; //unexpected
							}
						}
					}
					break;
				case 0x20://if cruise control speed strong reduction button was pressed, user wants to jump 10 pages forward
						if(wheelPressedButtonID==0x18 && baccableDashboardMenuVisible){ //if button released, use pressed button
							wheelPressedButtonID=0x20; //avoid to return here
							if(commandsMenuEnabled){
								switch(dashboard_menu_indent_level){
									case 0: //main menu
										main_dashboardPageIndex+= 1; //set next page
										if(function_read_faults_enabled==0){
											if(main_dashboardPageIndex==2) main_dashboardPageIndex++;
										}
										if(function_clear_faults_enabled==0){
											if(main_dashboardPageIndex==3) main_dashboardPageIndex++;
										}

										if(function_dyno_mode_master_enabled==0){
											if(main_dashboardPageIndex==5) main_dashboardPageIndex++;
										}
										if(function_esc_tc_customizator_enabled==0){
											if(main_dashboardPageIndex==6) main_dashboardPageIndex++;
										}
										if(function_front_brake_forcer_master==0){
											if(main_dashboardPageIndex==7) main_dashboardPageIndex++;
										}

										if(function_4wd_disabler_enabled==0){
											if(main_dashboardPageIndex==8) main_dashboardPageIndex++;
										}

										if(HAS_function_enabled==0){
											if(main_dashboardPageIndex==11) main_dashboardPageIndex++;
										}

										if(QV_exhaust_flap_function_enabled==0){
											if(main_dashboardPageIndex==12) main_dashboardPageIndex++;
										}

										if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=0; // make a rotative menu
										//onboardLed_blue_on();
										sendMainDashboardPageToSlaveBaccable();//send dashboard page to BH
										break;
									case 1:
										if(main_dashboardPageIndex==1){ //we are in show params submenu
											dashboardPageIndex += 10; //set 10 pages forward (+1 in gentle command)
											if(function_is_diesel_enabled==1){
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // make a rotative menu
											}else{
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // make a rotative menu
											}
											dashboardPageIndex=getNextVisibleParam(dashboardPageIndex);
												//onboardLed_blue_on();
											dashboardParamCouple[0]=NAN;//zeroize params
											dashboardParamCouple[1]=NAN;//zeroize params
											sendDashboardPageToSlaveBaccable(); //send dashboard page to BH
										}
										if(main_dashboardPageIndex==9){ //we are in setup menu
											setup_move_page(10);
											sendSetupDashboardPageToSlaveBaccable(); //send
										}
										if(main_dashboardPageIndex==10){ //we are in params setup menu
											params_setup_dashboardPageIndex+=10;//set next page
											if(params_setup_dashboardPageIndex>total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=0; // make a rotative menu
											//onboardLed_blue_on();
											sendParamsSetupDashboardPageToSlaveBaccable(); //send
										}
										break;
									default:
										break; //unexpected
								}
							}
						}

						break;
				case 0x08: //if cruise control speed increase button was pressed, user wants to see previous page
					if(wheelPressedButtonID==0x10 && baccableDashboardMenuVisible){ //if button released, use pressed button
						wheelPressedButtonID=0x08; //avoid to enter again here
						if(commandsMenuEnabled){
							switch(dashboard_menu_indent_level){
								case 0: //main menu
									main_dashboardPageIndex-= 1; //set next page

									if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=dashboard_main_menu_array_len-1; // make a rotative menu

									if(QV_exhaust_flap_function_enabled==0){
										if(main_dashboardPageIndex==12) main_dashboardPageIndex--;
									}

									if(HAS_function_enabled==0){
										if(main_dashboardPageIndex==11) main_dashboardPageIndex--;
									}


									if(function_4wd_disabler_enabled==0){
										if(main_dashboardPageIndex==8) main_dashboardPageIndex--;
										}

									if(function_front_brake_forcer_master==0){
										if(main_dashboardPageIndex==7) main_dashboardPageIndex--;
									}
									if(function_esc_tc_customizator_enabled==0){
										if(main_dashboardPageIndex==6) main_dashboardPageIndex--;
									}
									if(function_dyno_mode_master_enabled==0){
										if(main_dashboardPageIndex==5) main_dashboardPageIndex--;
									}

									if(function_clear_faults_enabled==0){
										if(main_dashboardPageIndex==3) main_dashboardPageIndex--;
									}
									if(function_read_faults_enabled==0){
										if(main_dashboardPageIndex==2) main_dashboardPageIndex--;
									}
									//onboardLed_blue_on();
									sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
									break;
								case 1:
									if(main_dashboardPageIndex==1){ //we are in show params submenu
										dashboardPageIndex -= 1; //set previous page
										if(function_is_diesel_enabled==1){
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=total_pages_in_dashboard_menu_diesel-1; // make a rotative menu
										}else{
											if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=total_pages_in_dashboard_menu_gasoline-1; // make a rotative menu
										}
										dashboardPageIndex=getPreviousVisibleParam(dashboardPageIndex);
										//onboardLed_blue_on();
										dashboardParamCouple[0]=NAN;//zeroize params
										dashboardParamCouple[1]=NAN;//zeroize params
										sendDashboardPageToSlaveBaccable(); //send dashboard page to BH
									}
									if(main_dashboardPageIndex==9){ //we are in setup menu
										setup_move_page(-1);
										sendSetupDashboardPageToSlaveBaccable(); //send
									}
									if(main_dashboardPageIndex==10){ //we are in params setup menu
										params_setup_dashboardPageIndex-=1;//set next page
										if(params_setup_dashboardPageIndex>total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=total_pages_in_params_setup_dashboard_menu; // make a rotative menu
										//onboardLed_blue_on();
										sendParamsSetupDashboardPageToSlaveBaccable(); //send
									}
									break;
								default:
									break; //unexpected
							}
						}
					}
					break;
				case 0x00: //if cruise control speed strong increase button was pressed, user wants to jump 10 pages before
						if(wheelPressedButtonID==0x08 && baccableDashboardMenuVisible){
							wheelPressedButtonID=0x00; //avoid to return here
							if(commandsMenuEnabled){
								switch(dashboard_menu_indent_level){
									case 0: //main menu
										main_dashboardPageIndex-= 1; //set next page
										if(main_dashboardPageIndex>=dashboard_main_menu_array_len)  main_dashboardPageIndex=dashboard_main_menu_array_len-1; // make a rotative menu

										if(QV_exhaust_flap_function_enabled==0){
											if(main_dashboardPageIndex==12) main_dashboardPageIndex--;
										}

										if(HAS_function_enabled==0){
											if(main_dashboardPageIndex==11) main_dashboardPageIndex--;
										}

										if(function_4wd_disabler_enabled==0){
											if(main_dashboardPageIndex==8) main_dashboardPageIndex--;
										}
										if(function_front_brake_forcer_master==0){
											if(main_dashboardPageIndex==7) main_dashboardPageIndex--;
										}
										if(function_esc_tc_customizator_enabled==0){
											if(main_dashboardPageIndex==6) main_dashboardPageIndex--;
										}
										if(function_dyno_mode_master_enabled==0){
											if(main_dashboardPageIndex==5) main_dashboardPageIndex--;
										}

										if(main_dashboardPageIndex==4) main_dashboardPageIndex--;

										if(function_clear_faults_enabled==0){
											if(main_dashboardPageIndex==3) main_dashboardPageIndex--;
										}
										if(function_read_faults_enabled==0){
											if(main_dashboardPageIndex==2) main_dashboardPageIndex--;
										}
										//onboardLed_blue_on();
										sendMainDashboardPageToSlaveBaccable();//send dashboard page via usb
										break;
									case 1:
										if(main_dashboardPageIndex==1){ //we are in show params submenu
											dashboardPageIndex -= 10; //set 10 pages backward

											if(function_is_diesel_enabled==1){
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_diesel)  dashboardPageIndex=0; // stay at zero.
											}else{
												if(dashboardPageIndex>=total_pages_in_dashboard_menu_gasoline)  dashboardPageIndex=0; // stay at zero.
											}
											dashboardPageIndex=getPreviousVisibleParam(dashboardPageIndex);


											//onboardLed_blue_on();
											dashboardParamCouple[0]=NAN;//zeroize params
											dashboardParamCouple[1]=NAN;//zeroize params
											sendDashboardPageToSlaveBaccable(); //send dashboard page to BH
										}
										if(main_dashboardPageIndex==9){ //we are in setup menu
											setup_move_page(-10);
											sendSetupDashboardPageToSlaveBaccable(); //send
										}
										if(main_dashboardPageIndex==10){ //we are in params setup menu
											params_setup_dashboardPageIndex-=10;//set prev page
											if(params_setup_dashboardPageIndex>total_pages_in_params_setup_dashboard_menu)  params_setup_dashboardPageIndex=0; // make a rotative menu
											//onboardLed_blue_on();
											sendParamsSetupDashboardPageToSlaveBaccable(); //send
										}
										break;
									default:
										break; //unexpected
								}
							}
						}
						break;
				case 0x10: // button released
					if(wheelPressedButtonID==0x89 && baccableDashboardMenuVisible==1){ //we pressed RES for at least one instant, then we released before 2 seconds, therefore we want to enter inside dashboard menu (will work only if menu is visible)

						if(dashboard_menu_indent_level==0){
							switch(main_dashboardPageIndex){
								case 1: //show params
									dashboard_menu_indent_level++;
									dashboardParamCouple[0]=NAN;//zeroize params
									dashboardParamCouple[1]=NAN;//zeroize params
									sendDashboardPageToSlaveBaccable();
									break;
								case 2: //read faults
									//To Be Done
									break;
								case 3: //clear faults
									clearFaultsRequest=255;
									break;
								case 4: //Immo
									// nothing to do
									break;
								case 5: // toggle dyno status
									//send request thu serial line
									if(carSteadyCounter>=100){ //car is steady since at least 1 second
										uint8_t tmpArr1[2]={C2BusID,C2cmdtoggleDyno};
										addToUARTSendQueue(tmpArr1, 2);
									}else{
										printStopTheCar=2;//print message "stop the car"
									}
									break;
								case 6: //toggle ESC/TC
									//send request thu serial line
									uint8_t tmpArr2[2]={C2BusID,C2cmdtoggleEscTc};
									addToUARTSendQueue(tmpArr2, 2);
									break;
								case 7: //toggle front brake

									if(front_brake_forced>0){ //toggle front brake
										if(launch_assist_enabled==1){ //if assist is enabled (by default it is enabled)
											launch_assist_enabled=0; //disable assist
										}else{ //launch assist is not enabled, but brakes are forced
											//send serial message to C2 baccable, to RELEASE front brakes
											uint8_t tmpArr3[2]={C2BusID,C2cmdNormalFrontBrake};
											addToUARTSendQueue(tmpArr3, 2);
										}
									}else{
										if(currentSpeed_km_h==0){
											if(!DynoModeEnabledOnMaster){
												printEnableDyno=2; //print message Enable DYNO
											}else{
												/* This commented part is a test... not working now


												//if dyno function is enabled but dyno is not enabled, enable dyno
												if(function_dyno_mode_master_enabled){
													if(DynoModeEnabledOnMaster==0){
														//send serial message to C2 baccable, to enable dyno
														uint8_t tmpArr4[2]={C2BusID,C2cmdForceFrontBrake};
														addToUARTSendQueue(tmpArr4, 2);
													}
												}

												//if 4wd function is enabled but 4wd is not disabled, disable 4wd
												if(function_4wd_disabler_enabled){
													if(_4wd_disabled==0){
														//seend messages to disable	4wd
														//TBDone.......
													}
												}
												*/
												//send serial message to C2 baccable, to force front brakes
												uint8_t tmpArr5[2]={C2BusID,C2cmdForceFrontBrake};
												addToUARTSendQueue(tmpArr5, 2);
											}
										}else{
											printStopTheCar=2;//print message "stop the car"
										}
									}


									break;
								case 8: //toggle 4wd
									if(_4wd_disabled>0){ //toggle 4dw disable
										_4wd_disabled=0;
										//update text
										dashboard_main_menu_array[main_dashboardPageIndex][4]=' '; //enabled
										dashboard_main_menu_array[main_dashboardPageIndex][5]='E';
										dashboard_main_menu_array[main_dashboardPageIndex][6]='n';
										commandsMenuEnabled=1;//enable menu commands
									}else{
										if(carSteadyCounter>=100){ //car is steady since at least one second
											_4wd_disabled=4;
											//update text
											dashboard_main_menu_array[main_dashboardPageIndex][4]='D'; //disabled
											dashboard_main_menu_array[main_dashboardPageIndex][5]='i';
											dashboard_main_menu_array[main_dashboardPageIndex][6]='s';
											commandsMenuEnabled=0;//disable menu commands
										}else{
											printStopTheCar=2;//print message "stop the car"
										}
									}
									break;
								case 9: //setup menu
								case 10: //params setup menu
									dashboard_menu_indent_level++;
									break;
								case 11: //toggle HAS function
									HAS_buttonPressRequested=5;
									//inform Slave baccable C2
									uint8_t tmpArr3[2]={C2BusID,C2cmdToggleHas};
									addToUARTSendQueue(tmpArr3, 2);
									break;
								case 12: //toggle QV Exhaust Valve
									if(ForceQVexhaustValveOpened==0){ //if valves are closed,
										ForceQVexhaustValveOpened=1; //start override sequence
									}else{
										ForceQVexhaustValveOpened=4; //return control to ecu
									}

									//equivalent activity for chinese valves
									if (chineseValveIsOpened==0){ //if valves are closed,
									ChineseExhaustValveRequest='O'; //open request
									}else{
									ChineseExhaustValveRequest='C'; //close request
									}

									break;
								case 13: //save Log to filesystem
									//inform Slave baccable C2 and BH
									uint8_t tmpArr4[2]={C2_Bh_BusID,C2_Bh_cmdFunction_Save_Log_to_File};
									addToUARTSendQueue(tmpArr4, 2);

									break;
								case 14: //reset statistics
									resetStatisticsOnFlash();
								default:
									break;
							}

						}else{ //indent level >0
							switch(main_dashboardPageIndex){
								case 9: //setup menu
									setup_select_page(setup_dashboardPageIndex);
									sendSetupDashboardPageToSlaveBaccable();
									break;
								case 10: //PARAMS SETUP MENU
									switch(params_setup_dashboardPageIndex){
										case 0: //{'S','A','V','E','&','E','X','I','T',},
											saveShownParamsOnflash();
											dashboard_menu_indent_level=0;
											break;
										default: //toggle hidden variable of current param
											shownParamsArray[params_setup_dashboardPageIndex-1]=!shownParamsArray[params_setup_dashboardPageIndex-1];
											break;
									}
									break;
								default:
									//we want to return main menu
									dashboard_menu_indent_level=0;
									sendMainDashboardPageToSlaveBaccable(); //print menu
							}
						}



					}
					wheelPressedButtonID=0x10; //button released
					lastPressedWheelButtonDuration=0;
					lastPressedSpeedUpWheelButtonDuration=0;
					break;
				case 0x90: //RES button was pressed
				case 0x50: //distance selector, used like RES, to manage the menu
					#ifndef HIDE_DASHBOARD_MENU
						lastPressedWheelButtonDuration++;

						if (wheelPressedButtonID==0x10 && wheelPressedButtonID!=0x90){//we pressed RES for at least one instant
							wheelPressedButtonID=0x89; //avoid returning here until button is not released
						}
						if (wheelPressedButtonID==0x89 && (lastPressedWheelButtonDuration>50)){//we pressed RES for around 2 seconds, therefore we want to enable/disable Baccable menu on dashboard
							wheelPressedButtonID=0x90; //avoid returning here until button is not released

							baccableDashboardMenuVisible=!baccableDashboardMenuVisible; //toggle visualizazion of the menu

							//LOG("Dashboard vis: %d\r\n", baccableDashboardMenuVisible);

							if(!baccableDashboardMenuVisible){ //if menu needs to be hidden, print spaces to clear the string on dashboard
								clearDashboardBaccableMenu();
							}else{
								//dashboardPageIndex=0; //reset the page, just to be sure to show initial Baccable print
								//main_dashboardPageIndex=0; //shows initial baccable version
								//dashboard_menu_indent_level=0;
							}


						}
					#endif

					break;
				case 0x12: //Cruise Control Disabled/Enabled
					break;
				default:
			}
		}

		#ifndef PERMANENTLY_DISABLE_IMMO
			if(cruiseControlDisabled && ACC_Disabled ){ //if we are allowed to use the buttons of the cruise control
				if (currentRpmSpeed>400){ //if motor is on
					if(currentGear==0){ //gear is neutral
						if((rx_msg_data[0]==0x08) && ((wheelPressedButtonID==0x10) || (wheelPressedButtonID==0x08))){ //user is pressing CC soft speed up button and it was previously released (or pressed by baccable menu up here)
							lastPressedSpeedUpWheelButtonDuration++;
							if(lastPressedSpeedUpWheelButtonDuration>1267){ //around 30 seconds
								//avoid to return here
								wheelPressedButtonID=0xF8; //invent a new status to differentiate it from 0x08 used in baccable menu few lines of code up here
								lastPressedSpeedUpWheelButtonDuration=0; //unuseful here since it is done when button is released. just to be superstitious :-D.
								immobilizerEnabled=!immobilizerEnabled;//toggle immobilizer status
								floodTheBus=0; //ensure to reset this even if probably it is not needed
								if(saveOnflash()>253){ //if we get error while permanently storeing the parameter on flash
									immobilizerEnabled=!immobilizerEnabled;//toggle immobilizer status to the original status and avoid to report the user anything
									onboardLed_red_on(); //a problem occurred
								}else{
									onboardLed_blue_on(); //everything goes fine. change saved on flash
									if(immobilizerEnabled){ //if immo enabled
										executeDashboardBlinks=6; //blinks the dashboard brightness 3 times
									}else{
										executeDashboardBlinks=12; //blinks the dashboard brightness 6 times
									}
								}


							}
						}
						if(rx_msg_data[0]==0x10){ //user released the button
							lastPressedSpeedUpWheelButtonDuration=0;
							wheelPressedButtonID=0x10; //button released
						}
					}

				}
			}
		#endif
	#endif

}
