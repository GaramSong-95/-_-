# 무인택배보관함시스템
아두이노 UNO,STM32F411RE 보드, 라즈베리파이 4를 활용하여 무인택배보관함 시스템 구현


![image](https://github.com/user-attachments/assets/747c0882-78b4-472f-9317-879b316be760)

![택배구조도 drawio](https://github.com/user-attachments/assets/31b0e600-50b6-43aa-a5db-5af97e743ab1)

## 🚀 사용한 기술 스택
<p align="left"> <img src="https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white" alt="Arduino" /> <img src="https://img.shields.io/badge/STM32-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white" alt="STM32" /> <img src="https://img.shields.io/badge/RaspberryPi-C51A4A?style=for-the-badge&logo=raspberrypi&logoColor=white" alt="Raspberry Pi" /> <img src="https://img.shields.io/badge/MariaDB-003545?style=for-the-badge&logo=mariadb&logoColor=white" alt="MariaDB" /> <img src="https://img.shields.io/badge/C/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C/C++" /> <img src="https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white" alt="Python" /> <img src="https://img.shields.io/badge/Android-3DDC84?style=for-the-badge&logo=android&logoColor=white" alt="Android" /> </p>



## 1.프로젝트 개요

**본 프로젝트는 택배 수령 과정에서 도난 방지와 효율적인 물류 운영을 위해 무인 택배 보관함 시스템을 구축하는 것을 목표로 하였습니다. 각 지역의 주요 스팟에 무인 택배 보관함을 설치하여, 사용자가 원하는 지역에서 프라이빗하게 택배를 수령할 수 있도록 설계하였습니다. Arduino Uno 보드를 활용하여 택배 보관함을 제어하고, 물류센터는 STM32 보드로 구현하였으며, 라즈베리파이 4에 서버를 구축하였고 wifi모듈과 bluetooth모듈을 활용한 통신을 구축하여 전체 시스템을 관리하도록 설계하였습니다.**

## 2.시스템 구성
본 프로젝트에서 저는 택배보관함 동작을 제외한 전체 시스템을 구축하였습니다. 주요 구성은 다음과 같습니다.
### ●서버 (Raspberry Pi 4)

라즈베리파이 4를 서버로 구축하여 전체 시스템을 관리.

클라이언트는 [ID]명령@명령@명령 형식의 메시지를 주고받으며, 특정 클라이언트 ID에 명령을 전송하면 해당 클라이언트가 명령을 수행.

MariaDB를 활용하여 물류 데이터를 저장 및 관리.

서버에 접속 가능한 주요 클라이언트:

SQL 클라이언트 (DB 관리)

택배 보관함 클라이언트 (Arduino Uno 기반)

물류센터 클라이언트 (STM32 기반/bluetooth활용용)

택배기사 및 사용자 (Android 앱 기반)

### ●데이터베이스 (MariaDB)

저장되는 데이터:

배송 상품 정보: 상품 ID, 배정된 보관함, 배정된 택배기사, 사용자 ID, 상품명, 전달 지역, 반송 여부, 배달 여부

택배기사 정보: 배정된 상품, 보관함, 택배기사 ID, 비밀번호

택배 보관함 정보: 보관함 ID, 상품 보관 여부, 배정된 사용자, 택배기사 ID, 지역

사용자 정보: 사용자 ID, 비밀번호, 보관함 ID, 상품 정보

배송 상태가 변경될 때마다 데이터 갱신.

### ●택배 보관함 시스템 (Arduino Uno)

wifi 모듈을 활용하여 라즈베리파이4와 통신 구축.

Arduino Uno 보드를 사용하여 택배 보관함 제어.

클라이언트 메시지를 수신하여 문 열기/닫기 명령 수행.

적외선 감지 센서로 문이 열린 상태를 판단하여 LED에 불이 켜짐.

온습도 센서를 통해 보관함 내부의 온습도를 측정하여 DB에 저장.

LCD디스플레이를 사용하여 보관함의 온습도 및 보관함 상태를 나타냄.

상태 정보 실시간 갱신 (보관함 사용 여부, 배정된 사용자 및 기사 ID 등).

### ●물류센터 시스템 (STM32)

bluetooth 모듈을 활용하여 라즈베리파이4와 통신 구축.

물류센터에서 STM32 보드를 사용하여 서버와 연동.

상품 배송 상황을 실시간으로 서버에 전송.

새로운 상품이 등록될 경우 DB 정보 업데이트 및 택배 운용 관리.

### ●모바일 애플리케이션 (Android 기반)

-택배기사

서버에 접속하여 SQL 클라이언트로부터 배송 상품 정보를 조회.

배송 시작 및 완료 처리.

기사 ID를 사용하여 보관함 문을 개방.

-사용자

서버에 접속하여 배송 도착 여부 확인.

배정된 보관함에서 상품 정보 확인 후 문 개방 가능.

SQL 클라이언트가 ID 일치 여부를 확인 후 문 개방 승인.

## 3.시스템 동작 흐름

물류센터에서 상품을 데이터베이스에 등록.

택배기사가 상품을 수령 후 무인택배보관함으로 배송.

보관함 클라이언트가 보관함 상태를 서버에 업데이트.

사용자는 앱을 통해 보관함 정보 확인 후 상품 수령.

택배가 수령되거나 반품될 경우, 모든 데이터베이스 정보가 갱신됨.

## 4.트러블슈팅 및 배운 점

본 프로젝트에서 처음으로 서버를 직접 구현하면서 코드 학습부터 어려움을 겪었습니다. 특히, 서버를 통해 클라이언트가 주고받는 메시지를 특정 기호에 맞게 분리하여 처리하는 과정이 복잡하게 느껴졌으며, 블루투스와 와이파이 통신 구현과 MariaDB에 정보를 업데이트하는 코드 작성 또한 큰 도전이었습니다. 하지만 끝까지 포기하지 않고 학습을 지속하여 결국에는 모든 코드를 이해하고 원하는 결과물을 구현할 수 있었습니다. 이 과정에서 문제 해결 능력을 키우고 개발자로서 한 단계 성장하는 계기가 되었습니다.

## 5.결론

본 프로젝트를 통해 무인 택배 보관함 시스템을 구현하였으며, 이를 통해 효율적인 물류 관리 및 사용자 편의성을 극대화하였습니다. 서버, 데이터베이스, 보관함 제어, 모바일 애플리케이션이 유기적으로 연동되어 원활한 운영이 가능하도록 설계하였습니다.

