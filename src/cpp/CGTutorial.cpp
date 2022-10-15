// Include Standardheader, steht bei jedem C/C++-Programm am Anfang
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW, GLEW ist ein notwendiges �bel. Der Hintergrund ist, dass OpenGL von Microsoft
// zwar unterst�tzt wird, aber nur in einer Uralt-Version. Deshalb beinhaltet die Header-Datei,
// die vom Betriebssystem zur Verf�gung gestellt wird, nur Deklarationen zu den uralten Funktionen,
// obwohl der OpenGL-Treiber, und damit die OpenGL-dll die neuesten Funktionen implementiert.
// Die neueren Funktionen werden deshalb �ber diese Header-Datei separat zur Verf�gung gestellt.
#include <GL/glew.h>

// Include GLFW, OpenGL definiert betriebssystemunabh�ngig die graphische Ausgabe. Interaktive 
// Programme be�tigen aber nat�rlich auch Funktionen f�r die Eingabe (z. B. Tastatureingaben)
// Dies geht bei jedem OS (z. B. Windows vs. MacOS/Unix) etwas anders. Um nun generell plattformunabh�ngig
// zu sein, verwenden wir GLFW, was die gleichen Eingabe-Funktionen auf die Implementierung unterschiedlicher
// OS abbildet. (Dazu gibt es Alternativen, glut wird z. B. auch h�ufig verwendet.)
#include <GLFW/glfw3.h>

// Include GLM, GLM definiert f�r OpenGL-Anwendungen Funktionen der linearen Algebra wie
// Transformationsmatrizen. Mann k�nnte GLM auch durch etaws anderes ersetzen oder aber in einem
// anderen Kontext verwenden.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// In C- und C++-Programmen ist die Reihenfolge der include-Direktiven wichtig.
// Dateien, die mit Spitzklammern includiert werden, werden in den System-Verzeichnissen
// gesucht, die mit doppelten Hochkommata im lokalen Projektverzeichnis
// (wo genau ist in den Projekteinstellungen zu finden und ggf. zu �ndern.) 

// Diese Datei ben�tigen wir, um die Shader-Programme komfortabel in die Hardware zu laden.
// (Mit der rechten Mouse-taste k�nnen Sie in VS diese Datei �ffnen, um nachzuschauen, was dort deklariert wird.)
#include "shader.hpp"

// Wuerfel und Kugel
#include "objects.hpp"

// kuemmert sich um den Pfad zu den Shadern
#include "asset.hpp"

// Callback-Mechanismen gibt es in unterschiedlicher Form in allen m�glichen Programmiersprachen,
// sehr h�ufig in interaktiven graphischen Anwendungen. In der Programmiersprache C werden dazu 
// Funktionspointer verwendet. Man �bergibt einer aufgerufenen Funktion einer Bibliothek einen
// Zeiger auf eine Funktion, die zur�ckgerufen werden kann. Die Signatur der Funktion muss dabei
// passen. Dieser Mechanismus existiert auch in C++ und wird hier verwendet, um eine einfache
// Fehlerbehandlung durchzuf�hren. Diese Funktion gibt Fehler aus, die beim Aufruf von OpenGL-Befehlen
// auftreten.
void error_callback(int error, const char* description)
{
	// Mit fputs gibt man hier den String auf den Standarderror-Kanal aus.
	// In der C-Welt, aus der das hier �bernommen ist, sind Strings Felder aus "char"s, die mit 
	// dem Wert null terminiert werden.
	fputs(description, stderr);
}

// Diese Funktion wird ebenfalls �ber Funktionspointer der GLFW-Bibliothek �bergeben.
// (Die Signatur ist hier besonders wichtig. Wir sehen, dass hier drei Parameter definiert
//  werden m�ssen, die gar nicht verwendet werden.)
// Generell �berlassen wir der GLFW-Bibliothek die Behandlung der Input-Ereignisse (Mouse moved,
// button click, Key pressed, etc.).
// Durch die �bergabe dieser Funktion k�nnen wir Keyboard-Events 
// abfangen. Mouse-Events z. B. erhalten wir nicht, da wir keinen Callback an GLFW �bergeben.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	// Mit rechte Mousetaste -> gehe zu Deklaration finden Sie andere Konstanten f�r Tasten.
	case GLFW_KEY_ESCAPE:
		// Das Programm wird beendet, wenn BenutzerInnen die Escapetaste bet�tigen.
		// Wir k�nnten hier direkt die C-Funktion "exit" aufrufen, eleganter ist aber, GLFW mitzuteilen
		// dass wir das Fenster schliessen wollen (siehe Schleife unten).
		glfwSetWindowShouldClose(window, GL_TRUE);
		break;

	default:
		break;
	}
}


// Diese drei Matrizen speichern wir global (Singleton-Muster), damit sie jederzeit modifiziert und
// an die Grafikkarte geschickt werden koennen. Ihre Bedeutung habe ich in der Vorlesung Geometrische
// Transformationen erkl�rt, falls noch nicht geschehen, jetzt anschauen !
glm::mat4 Projection;
glm::mat4 View;
glm::mat4 Model;
GLuint programID; // OpenGL unterst�tzt unterschiedliche Shaderprogramme, zwischen denen man
                  // wechseln kann. Unser Programm wird mit der unsigned-integer-Variable programID
                  // referenziert.

// Ich habe Ihnen hier eine Hilfsfunktion definiert, die wir verwenden, um die Transformationsmatrizen
// zwischen dem OpenGL-Programm auf der CPU und den Shaderprogrammen in den GPUs zu synchronisieren.
// (Muss immer aufgerufen werden, bevor wir Geometriedaten in die Pipeline einspeisen.)
void sendMVP()
{
	// Zun�chst k�nnen wir die drei Matrizen einfach kombinieren, da unser einfachster Shader
	// wirklich nur eine Transformationsmatrix ben�tigt, wie in der Vorlesung erkl�rt.
	// Sp�ter werden wir hier auch die Teilmatrizen an den Shader �bermitteln m�ssen.
	// Interessant ist hier, dass man in C++ (wie auch in C#) den "*"-Operator �berladen kann, so dass
	// man Klassenobjekte miteinander multiplizieren kann (hier Matrizen bzw. "mat4"), 
	// das ginge in JAVA so nat�rlich nicht. 
	glm::mat4 MVP = Projection * View * Model;

	// "glGetUniformLocation" liefert uns eine Referenz auf eine Variable, die im Shaderprogramm
	// definiert ist, in diesem Fall heisst die Variable "MVP".
	// "glUniformMatrix4fv" �bertr�gt Daten, genauer 4x4-Matrizen, aus dem Adressraum unserer CPU
	// (vierter Parameter beim Funktionsaufruf, wir generieren mit "&" hier einen Pointer auf das erste 
	//  Element, und damit auf das gesamte Feld bzw den Speicherbereich) 
	// in den Adressraum der GPUs. Beim ersten Parameter 
	// muss eine Referenz auf eine Variable im Adressraum der GPU angegeben werden.
	glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
}

// Einstiegspunkt f�r C- und C++-Programme (Funktion), Konsolenprogramme k�nnte hier auch Parameter erwarten
int main(void)
{
	// Initialisierung der GLFW-Bibliothek
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	// Fehler werden auf stderr ausgegeben, s. o.
	glfwSetErrorCallback(error_callback);

	// �ffnen eines Fensters f�r OpenGL, die letzten beiden Parameter sind hier unwichtig
	// Diese Funktion darf erst aufgerufen werden, nachdem GLFW initialisiert wurde.
	// (Ggf. glfwWindowHint vorher aufrufen, um erforderliche Resourcen festzulegen -> MacOSX)
	GLFWwindow* window = glfwCreateWindow(1024, // Breite
										  768,  // Hoehe
										  "CG - Tutorial", // Ueberschrift
										  NULL,  // windowed mode
										  NULL); // shared window

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Wir k�nnten uns mit glfwCreateWindow auch mehrere Fenster aufmachen...
	// Sp�testens dann w�re klar, dass wir den OpenGL-Befehlen mitteilen m�ssen, in
	// welches Fenster sie "malen" sollen. Wir m�ssen das aber zwingend auch machen,
	// wenn es nur ein Fenster gibt.

	// Bis auf weiteres sollen OpenGL-Befehle in "window" malen.
	// Ein "Graphic Context" (GC) speichert alle Informationen zur Darstellung, z. B.
	// die Linienfarbe, die Hintergrundfarbe. Dieses Konzept hat den Vorteil, dass
	// die Malbefehle selbst weniger Parameter ben�tigen.
	// Erst danach darf man dann OpenGL-Befehle aufrufen !
    glfwMakeContextCurrent(window);

	// Initialisiere GLEW
	// (GLEW erm�glicht Zugriff auf OpenGL-API > 1.1)
	glewExperimental = true; // Diese Zeile ist leider notwendig.

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Auf Keyboard-Events reagieren (s. o.)
	glfwSetKeyCallback(window, key_callback);

	// Setzen von Dunkelblau als Hintergrundfarbe (erster OpenGL-Befehl in diesem Programm).
	// Beim sp�teren L�schen gibt man die Farbe dann nicht mehr an, sondern liest sie aus dem GC
	// Der Wertebereich in OpenGL geht nicht von 0 bis 255, sondern von 0 bis 1, hier sind Werte
	// fuer R, G und B angegeben, der vierte Wert alpha bzw. Transparenz ist beliebig, da wir keine
	// Transparenz verwenden. Zu den Farben sei auf die entsprechende Vorlesung verwiesen !
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Kreieren von Shadern aus den angegebenen Dateien, kompilieren und linken und in
	// die Grafikkarte �bertragen.  
	programID = LoadShaders(SHADER_DIR "/TransformVertexShader.vertexshader", SHADER_DIR "/ColorFragmentShader.fragmentshader");

	// Diesen Shader aktivieren ! (Man kann zwischen Shadern wechseln.) 
	glUseProgram(programID);

	// Alles ist vorbereitet, jetzt kann die Eventloop laufen...
	while (!glfwWindowShouldClose(window))
	{
		// L�schen des Bildschirms (COLOR_BUFFER), man kann auch andere Speicher zus�tzlich l�schen, 
		// kommt in sp�teren �bungen noch...
		// Per Konvention sollte man jedes Bild mit dem L�schen des Bildschirms beginnen, muss man aber nicht...
		glClear(GL_COLOR_BUFFER_BIT);	

		// Einstellen der Geometrischen Transformationen
		// Wir verwenden dazu die Funktionen aus glm.h
		// Projektionsmatrix mit 45Grad horizontalem �ffnungswinkel, 4:3 Seitenverh�ltnis, 
		// Frontplane bai 0.1 und Backplane bei 100. (Das sind OpenGL-Einheiten, keine Meter oder der gleichen.)
		Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
		
		// Viewmatrix, beschreibt wo die Kamera steht, wo sie hinschaut, und wo oben ist. 
		// Man muss angeben, wo oben ist, da es eine Mehrdeutigkeit g�be, wenn man nur beschreiben
		// w�rde, wo die Kamera steht und wo sie hinschaut. Denken Sie an ein Flugzeug. Die Position 
		// des/r Piloten/in in der Welt ist klar, es ist dann auch klar, wo er/sie hinschaut. Das Flugzeug 
		// kann sich aber z. B. auf die Seite legen, dann w�rde der Horizont "kippen". Dieser Aspekt wird
		// mit dem up-Vektor (hier "oben") gesteuert.
		View = glm::lookAt(glm::vec3(0,0,-5), // die Kamera ist bei (0,0,-5), in Weltkoordinaten
						   glm::vec3(0,0,0),  // und schaut in den Ursprung
						   glm::vec3(0,1,0)); // Oben ist bei (0,1,0), das ist die y-Achse
	
		
		// Modelmatrix : Hier auf Einheitsmatrix gesetzt, was bedeutet, dass die Objekte sich im Ursprung
		// des Weltkoordinatensystems befinden.
		Model = glm::mat4(1.0f);

		// Diese Informationen (Projection, View, Model) m�ssen geeignet der Grafikkarte �bermittelt werden,
		// damit sie beim Zeichnen von Objekten ber�cksichtigt werden k�nnen.
		sendMVP();

		// Nachdem der GC in der Grafikkarte aktuell ist, also z. B. auch ein sendMVP ausgef�hrt wurde,
		// zeichen wir hier nun einen W�rfel. Dazu werden in "drawWireCube" die Eckpunkte zur Grafikkarte 
		// geschickt. Der gew�hlte Modus legt fest, wie die Punkte mit Linien verbunden werden.
		// Das werden wir uns sp�ter noch genauer anschauen. (Schauen Sie sich die schwarzen Linien genau an,
		// und �berlegen Sie sich, dass das wirklich ein W�rfel ist, der perspektivisch verzerrt ist.)
		// Die Darstellung nennt man �brigens "im Drahtmodell".
		drawWireCube();		

		// Bildende. 
		// Bilder werden in den Bildspeicher gezeichnet (so schnell wie es geht.). 
		// Der Bildspeicher wird mit der eingestellten Bildwiederholfrequenz (also z. B. 60Hz)
		// ausgelesen und auf dem Bildschirm dargestellt. Da beide Frequenzen nicht �bereinstimmen, w�rde
		// man beim Auslesen auf unfertige Bilder sto�en. Das w�re als Flimmern auf dem Bildschirm zu
		// erkennen. (War bei �lteren Grafikkarten tats�chlich so.)
		// Dieses Problem vermeidet man, wenn man zwei Bildspeicher benutzt, wobei in einen gerade
		// gemalt wird, bzw. dort ein neues Bild entsteht, und der andere auf dem Bildschirm ausgegeben wird.
		// Ist man mit dem Erstellen eines Bildes fertig, tauscht man diese beiden Speicher einfach aus ("swap").
		glfwSwapBuffers(window);

		// Hier fordern wir glfw auf, Ereignisse zu behandeln. GLFW k�nnte hier z. B. feststellen,
		// das die Mouse bewegt wurde und eine Taste bet�tigt wurde.
		// Da wir zurzeit nur einen "key_callback" installiert haben, wird dann nur genau diese Funktion
		// aus "glfwPollEvents" heraus aufgerufen.
        glfwPollEvents();
	} 

	// Wenn der Benutzer, das Schliesskreuz oder die Escape-Taste bet�tigt hat, endet die Schleife und
	// wir kommen an diese Stelle. Hier k�nnen wir aufr�umen, und z. B. das Shaderprogramm in der
	// Grafikkarte l�schen. (Das macht zurnot das OS aber auch automatisch.)
	glDeleteProgram(programID);

	// Schie�en des OpenGL-Fensters und beenden von GLFW.
	glfwTerminate();

	return 0; // Integer zur�ckgeben, weil main so definiert ist
}

