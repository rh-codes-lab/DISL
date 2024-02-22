with open('test.hex') as f:
    hexdata = f.readlines()
hexdigits=['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F']
hexdata2 = []
results = []
frame = ''
counter = 0
for i in range(len(hexdata)):
    if counter == 28:
        counter = 0
        line = hexdata[i][:-1]
        results.append(line[-1])
    else:
        counter += 1
        line = hexdata[i][:-1]
        for j in range(len(line)):
            idx = len(line)-j-1
            frame = line[idx] + frame
            if len(frame) == 49*2:
                frame = '00000000000000' + frame
                hexdata2.append(frame)
                frame = ''
counter = 0
counter2 = 0
counter3 = 0
print('uint32_t rows[][16][14] = {\n{',end='')
for frame in hexdata2:
    if counter3 == 16:
        print(',\n{',end='')
        counter3 = 0
    print("{",end='')
    for c in frame:
        if counter == 0:
            print('0x',end='')
        print(c,end='')
        counter += 1
        if counter == 8:
            if counter2 < 13:
                print(',',end='')
            counter2 += 1
            counter = 0
    counter = 0
    counter2 = 0
    print("}",end='')
    counter3 += 1
    if counter3 == 16:
        print("}",end='')
    else:
        print(',')
print("};")
print("uint8_t results[]= {" + ",".join(results) + "};")
print("#define NUM_TESTS " + str(len(results)))


