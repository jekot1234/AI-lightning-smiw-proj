import jetson.inference
import jetson.utils
import requests


dynamic_mode = 0
restart_needed = 0
net_type = "ssd-mobilenet-v2"
thershold = 0.5
no_connection = 1

camera = jetson.utils.gstCamera(1280, 720, "0")
display = jetson.utils.glDisplay()

while 1:


    restart_needed = 0

    #Searching for ESP loop
    while 1:
        print("Searching local network for ESP")
        for x in range(256):
            ip = "192.168.0." + str(x)
            address = "http://" + ip + "/establish"
            print(ip)
            try:
                request = requests.get(address, timeout=0.05)
                if(request.status_code == 200):
                    print("Connected with ESP at " + ip)
                    no_connection = 0
                    break
            except:
                no_connection = 1
        if not no_connection:
            break
        else:
            print("ESP not found")
    
    on = "http://" + ip + "/On"
    off = "http://" + ip + "/Off"

    #Settings fetch
    request = requests.get("http://" + ip + "/settings")
    settings = request.text.split()
    if settings[0]  == "dyn":
        dynamic_mode = 1
    else:
        dynamic_mode = 0
    if settings[1] == "mob1":
        net_type = "ssd-mobilenet-v1"
    elif settings[1] == "mob2":
        net_type = "ssd-mobilenet-v2"
    elif settings[1] == "ince":
        net_type = "ssd-inception-v2"
    threshold = float(settings[2])/100

    #Network initialization
    net = jetson.inference.detectNet(net_type, threshold = threshold)

    if dynamic_mode:

        person_frames = 0
        no_person_frames = 0
        mul_persons_frames = 0

        coord_sum = 0

        while 1:
            img, width, height = camera.CaptureRGBA()
            detections = net.Detect(img, width, height)
            display.RenderOnce(img, width, height)
            display.SetTitle("Projekt SMIW 2021 | {:.0f} FPS".format(net.GetNetworkFPS()))

            person = 0

            coord = 0

            for detection in detections:

                if(detection.ClassID == 1):
                    person += 1
                    coord += (detection.Left + detection.Right) / 2
                    print (coord)

            if person == 1:
                person_frames += 1
                coord_sum += coord
            elif person == 0:
                no_person_frames += 1
            else:
                mul_persons_frames += 1

            if person_frames == 5:
                step = int((coord_sum / 5)  / 128)
                print(step)

                try:
                    request = requests.get("http://" + ip + "/DynOn?step=" + str(step))
                    if request.status_code == 300:
                        print("Settings changed, restarting network now")
                        restart_needed = 1
                        break
                except:
                    print("Connection with ESP lost")
                    no_connection = 1
                    break

                person_frames = 0
                no_person_frames = 0
                mul_persons_frames = 0
                coord_sum = 0
            elif no_person_frames == 5:

                try:
                    request = requests.get("http://" + ip + "/DynOff")
                    if request.status_code == 300:
                        print("Settings changed, restarting network now")
                        restart_needed = 1
                        break
                except:
                    print("Connection with ESP lost")
                    no_connection = 1
                    break
                
                    person_frames = 0
                no_person_frames = 0
                mul_persons_frames = 0
                coord_sum = 0
                
            elif mul_persons_frames == 5:

                try:
                    request = requests.get("http://" + ip + "/DynMul")
                    if request.status_code == 300:
                        print("Settings changed, restarting network now")
                        restart_needed = 1
                        break
                except:
                    print("Connection with ESP lost")
                    no_connection = 1
                    break

                person_frames = 0
                no_person_frames = 0
                mul_persons_frames = 0
                coord_sum = 0
            if no_connection or restart_needed:
                break
    else:
        while 1:
            img, width, height = camera.CaptureRGBA()
            detections = net.Detect(img, width, height)
            display.RenderOnce(img, width, height)
            display.SetTitle("Projekt SMIW 2021 | {:.0f} FPS".format(net.GetNetworkFPS()))

            person = 0

            for detection in detections:

                if(detection.ClassID == 1):
                    person = 1
                    break


            if person:
                try:
                    request = requests.get(on)
                    if request.status_code == 300:
                        print("Settings changed, restarting network now")
                        restart_needed = 1
                        break
                except:
                    print("Connection with ESP lost")
                    no_connection = 1
                    break
            else:
                try:
                    request = requests.get(off)
                    if request.status_code == 300:
                        print("Settings changed, restarting network now")
                        restart_needed = 1
                        break
                except:
                    print("Connection with ESP lost")
                    no_connection = 1
                    break
            if no_connection or restart_needed:
                break

            
            
