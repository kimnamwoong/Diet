# Diet
삼성 헬스 어플을 벤치 마킹한 C++ Qt My SQL  활용한 프로그램 

프로젝트 기간
2021.02.15 ~ 2021.02.20


󰋼 설명
- 사용자가 추가한 음식에 대한 칼로리 및 영양소 정보를 계산해주고 하루동안 섭취한 칼로리 정보를 조회

󰋼 개발환경
- Ubuntu 20.04, 
- DB : MySQL Workbench 8.0.21
- 서버 : Visual Studio Code/ C++
- 클라이언트 : Qt Creator/ C++


󰋼 구현 파트
- 서버 기본틀 작성 및 서버 클라이언트 간 통신구현
- 클라이언트 기능 중 로그인,회원가입,내 정보 수정 구현



󰋼 프로그램 기능


1. 회원가입
입력한 사용자의 키, 몸무게, 성별 BMI와 하루 권장 칼로리계산

2. 음식정보 추가기능 
- 음식명선택, gram 을 입력하면 서버를 통해 DB에 데이터 추가

3. 내정보 수정
- 사용자의 키, 몸무게 수정

4. 운동
- 하루 섭취 칼로리를 소모하기 위한 걷기,뛰기,싸이클 운동량 조회

5. 권장섭취량
- 하루 권장 섭취량과 사용자가 섭취량 정보 조회



프로그램 구조도

![image](https://user-images.githubusercontent.com/94125986/153137841-f12a469d-e3d0-4d21-bf29-3a019f9fe40f.png)


DB 설계 

![image](https://user-images.githubusercontent.com/94125986/153137870-06a3cae7-c9d2-4d3e-bf62-4cead9b83798.png)


회원가입 & 중복확인

![image](https://user-images.githubusercontent.com/94125986/153137945-731ed8a5-631e-4570-9fd8-d628961638cb.png)

![image](https://user-images.githubusercontent.com/94125986/153137957-5e624dbc-4d2e-4537-8fd2-f95dc03ebbb5.png)


DB에 반영

![image](https://user-images.githubusercontent.com/94125986/153138020-b76f289c-cc62-4250-acef-fddb272084d2.png)


섭취 음식 정보 추가

![image](https://user-images.githubusercontent.com/94125986/153138069-e15842dd-b133-4aba-ab54-5b21847b2251.png)


선택한 음식에 대한 칼로리 및 영양소 정보 출력

![image](https://user-images.githubusercontent.com/94125986/153138093-ad717128-d4b1-4d52-aec3-131341b03796.png)


하루 섭취량 기준 운동정보 확인

![image](https://user-images.githubusercontent.com/94125986/153138120-ce24c72d-21d0-4912-a6e7-c74ad1bfc877.png)

- 각 운동의 10분당 소모칼로리를 통해 클라이언트가 섭취한 총 칼로리에 따른 운동정보 출력


내 정보 수정

![image](https://user-images.githubusercontent.com/94125986/153138182-cb7b4274-4446-4bc7-8a33-74b8e0d3f28e.png)

![image](https://user-images.githubusercontent.com/94125986/153138190-7e41b91f-01a1-4a36-a43a-1681e7f5a211.png)

- 정보 수정 후 DB상 데이터 update

![image](https://user-images.githubusercontent.com/94125986/153138213-1c5ac7fe-fc09-4dd0-a28c-637a48e576bd.png)



권장 섭취량 정보 출력
- 사용자의 키,몸무게,성별로 BMI계산
- BMI에 따른 일 권장섭취량 계산
- 사용자가 추가한 하루 섭취량과 권장섭취량 비교 

![image](https://user-images.githubusercontent.com/94125986/153138254-977524e0-c63e-48af-9828-ecf9df94ab97.png)

