/*
 * Tests.h
 *
 *  Created on: May 27, 2016
 *      Author: Md
 */

#ifndef TESTS_H_
#define TESTS_H_

	  void TestMoteurs(uint16_t vmax, uint16_t dv, uint16_t dt);
	  void TestMotX(uint16_t vmax, uint16_t dv, uint16_t dt);

	  void TestFrottementsSecs(uint16_t vmax, uint16_t dv, uint16_t dt);
	  void TestCodeurs(void);
	  void TestCodeurX(void);
	  void TestDist(unsigned dist);
	  void TestRot(unsigned ang);

	  void TestCaptCoul(void);
	  void Test5Servos(void);




	  void InitMesTemps(void);
	  uint32_t GetTps(void);
	  void StartTps(void);
	  uint32_t TpsToMs(uint32_t n);
	  uint32_t TpsToUs(uint32_t n);
	  uint32_t TpsToNs(uint32_t n);


	  void TEST1M(void);
	  void TEST360(void);
	  void SPINBOT(void);
	  void TestFrotSecs(void);
	  void TestFusee(void);
	  void DeploiMax(void);
	  void USDistAV100(void);
	  void USDistAV200(void);
	  void USDistAR100(void);
	  void USDistAR200(void);
	  void TESTAFFUS(void);
	  void TestIOnum(void);
	  void TESTPANNEAUX(void);

	  void AffPeriphI2C(void);








#endif /* TESTS_H_ */
