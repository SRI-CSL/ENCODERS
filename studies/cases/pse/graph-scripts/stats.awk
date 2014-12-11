BEGIN {
    sum_iq=0;
}
{
    sum_iq = sum_iq + $1;
    iq[NR] = $1;
    if (min == "") {
        min = max = $1
    }

    if ($1 > max) {
        max = $1;
    }

    if ($1 < min) {
        min = $1;
    }
}
END {
    mean_iq = 0;
    if (NR > 0) {
        mean_iq = sum_iq/NR;
    }
    stdd_iq = 0;
    for(i=1; i<=NR; i++) {
        stdd_iq += (iq[i] - mean_iq) * (iq[i] - mean_iq);
    }
    if (NR-1 > 0) {
        stdd_iq = sqrt(stdd_iq/(NR-1));
    }
    print mean_iq "," stdd_iq "," min "," max
}
