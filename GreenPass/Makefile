all: clean Client ClientS ClientT CentroVaccinale ServerG ServerV

Client: Client.o GreenPass.o LabUtilities.o
	gcc Client.o GreenPass.o LabUtilities.o -o Client.out

Client.o: Client.c GreenPass.h LabUtilities.h
	gcc -c Client.c

ClientS: ClientS.o GreenPass.o LabUtilities.o
	gcc ClientS.o GreenPass.o LabUtilities.o -o ClientS.out

ClientS.o: ClientS.c GreenPass.h LabUtilities.h
	gcc -c ClientS.c

ClientT: ClientT.o GreenPass.o LabUtilities.o
	gcc ClientT.o GreenPass.o LabUtilities.o -o ClientT.out

ClientT.o: ClientT.c GreenPass.h LabUtilities.h
	gcc -c ClientT.c

CentroVaccinale: CentroVaccinale.o GreenPass.o LabUtilities.o
	gcc CentroVaccinale.o GreenPass.o LabUtilities.o -o CentroVaccinale.out

CentroVaccinale.o: CentroVaccinale.c GreenPass.h LabUtilities.h
	gcc -c CentroVaccinale.c

ServerG: ServerG.o GreenPass.o LabUtilities.o
	gcc ServerG.o GreenPass.o LabUtilities.o -o ServerG.out

ServerG.o: ServerG.c GreenPass.h LabUtilities.h
	gcc -c ServerG.c

ServerV: ServerV.o GreenPass.o LabUtilities.o
	gcc ServerV.o GreenPass.o LabUtilities.o -o ServerV.out

ServerV.o: ServerV.c GreenPass.h LabUtilities.h
	gcc -c ServerV.c

GreenPass.o: GreenPass.c GreenPass.h LabUtilities.c LabUtilities.h
	gcc -c GreenPass.c LabUtilities.c

LabUtilities.o: LabUtilities.c LabUtilities.h
	gcc -c LabUtilities.c

clean:
	rm -f *.o
	rm -f *.out