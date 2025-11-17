# -*- coding: utf-8 -*-
"""
Created on Tue Jun 10 11:15:52 2025

@author: bohm
"""

import numpy as np
from scipy.stats import unitary_group
from scipy.optimize import fsolve


def get_unitary(dim: int):
    return unitary_group.rvs(dim)


def _U_times_BS_entry(
    angles: tuple[float, float],
    U,
    mode_1: int,
    mode_2: int,
    row: int,
    col: int,
    is_odd: bool,
):
    """If is_odd is True, multiply a row of the unitary U times a column of a
    beamsplitter's inverse. If is_odd is False, multiply a row of a beamsplitter
    times a column of the unitary U.

    Args:
        angles (float, float): angles and shift of the beamsplitter

    Returns:
        NDArray: the real and imaginary parts of the dot product
    """

    θ = angles[0]
    φ = angles[1]

    # Since T and T.inv() are sparse there is no need on creating them from scratch
    if is_odd:
        #print(U[row, mode_1], U[row, mode_2])
        # U[row,:] @ T.inv[:, col]
        dot_prod = U[row, mode_1] * 1j * np.exp(1j * θ) * np.exp(-1j * φ) * np.sin(θ) + U[row, mode_2] * np.cos(θ) * np.exp(1j * θ) * 1j
        #dot_prod = U[row, mode_1] * np.exp(-1j * φ) * np.sin(θ) + U[row, mode_2] * np.cos(θ)
    else:
        #print(U[mode_1, col], U[mode_2, col])
        # T[row,:] @ U[:, col]
        dot_prod = 1j * np.exp(1j * φ) * np.cos(θ) * U[mode_1, col]* np.exp(1j * θ) - 1j  * np.sin(θ) * U[mode_2, col]* np.exp(1j * θ)

    return np.array([dot_prod.real, dot_prod.imag])

"""
def atan2f(y, x, tolerance=1e-6, to_degree=False):
    zero_y = np.abs(y) <= tolerance
    zero_x = np.abs(x) <= tolerance
    if zero_x and zero_y:
        rad = 0
    elif zero_x and (not zero_y):
        rad = np.pi/2 if y > tolerance else -np.pi/2
    elif (not zero_x) and zero_y:
        rad = 0 if x > tolerance else np.pi
    else:
        rad = np.arctan2(y, x)
    if to_degree:
        return np.rad2deg(rad)
    else:
        return rad
"""    
"""
def angle_diff(comp_src, comp_dst, offset=0, tolerance=1e-6, wrap=True, to_degree=False):
    zero_src = np.abs(comp_src) <= tolerance
    zero_dst = np.abs(comp_dst) <= tolerance
    if zero_src and zero_dst:
        rad = 0
    elif zero_src and (not zero_dst):
        rad = np.angle(comp_dst)
    elif (not zero_src) and zero_dst:
        rad = -np.angle(comp_src)
    else:
        rad = np.angle(comp_dst) - np.angle(comp_src)
    rad += offset
    if wrap:
        rad = np.mod(rad, 2 * np.pi)
    if to_degree:
        return np.rad2deg(rad)
    else:
        return rad
"""

def U2MZI2(dim, m, n, phi, theta, Lp=1, Lc=1):
    assert m < n < dim
    mat = np.eye(dim, dtype=np.complex128)
    mat[m, m] = np.sqrt(Lp) * 1j * np.exp(1j * phi) * np.sin(theta)
    mat[m, n] = np.sqrt(Lc) * 1j * np.cos(theta)
    mat[n, m] = np.sqrt(Lc) * 1j * np.exp(1j * phi) * np.cos(theta)
    mat[n, n] = -np.sqrt(Lp) * 1j * np.sin(theta)
    return mat    

def U2MZI3(dim, m, n, phi, theta, Lp=1, Lc=1):
    assert m < n < dim
    mat = np.eye(dim, dtype=np.complex128)
    mat[m, m] = np.sqrt(Lp) * 1j * np.exp(1j * theta) * np.exp(1j * phi) * np.sin(theta)
    mat[m, n] = np.sqrt(Lc) * 1j * np.exp(1j * theta) * np.cos(theta)
    mat[n, m] = np.sqrt(Lc) * 1j * np.exp(1j * theta) * np.exp(1j * phi) * np.cos(theta)
    mat[n, n] = -np.sqrt(Lp) * 1j * np.exp(1j * theta) * np.sin(theta)
    return mat

def U2MZI(dim, m, n, phi, theta):
    assert m < n < dim
    
    coupler_matrix = np.eye(dim, dtype=np.complex128)
    phase_shifter = np.eye(dim, dtype=np.complex128)
    mat = np.eye(dim, dtype=np.complex128)

    coupler_matrix[m, m] = 1 / np.sqrt(2)
    coupler_matrix[m, n] = 1j / np.sqrt(2)
    coupler_matrix[n, m] = 1j / np.sqrt(2)
    coupler_matrix[n, n] = 1 / np.sqrt(2)
    
    mat = np.matmul(mat, coupler_matrix)
    
    phase_shifter[m, m] = np.exp(1j * theta * 2)
    mat = np.matmul(mat, phase_shifter)
    
    mat = np.matmul(mat, coupler_matrix)
    
    phase_shifter[m, m] = np.exp(1j * phi)
    mat = np.matmul(mat, phase_shifter)
        
    return mat

    
def decompose_clements(u):
    assert isinstance(u, np.ndarray)
    if len(u.shape) != 2:
        raise ValueError("U(N) should be 2-dimension matrix.")
        
    if u.shape[0] != u.shape[1]:
        raise ValueError("U(N) should be a square matrix.")
        
    mat = u.copy().astype(np.complex128)
    dim = mat.shape[0]
    
    row = dim - 1
    col = int(np.ceil(dim / 2))
    
    cnt_fore = np.zeros(row, dtype=int)
    cnt_back = np.ones(row, dtype=int) * (col - 1)
    if dim % 2 == 1:
        cnt_back[1::2] = col - 2
    
    phis = np.zeros((row, col))
    thetas = np.zeros((row, col))
    alphas = np.zeros(dim)
    U2block = U2MZI3

    
    for p in range(dim-1):
        for q in range(p+1):
            if p % 2 == 0: #right multiply
                x = dim - 1 - q
                y = p - q

                def null_U_entry(angles):
                    return _U_times_BS_entry(angles, mat, y, y+1, x, y, is_odd = True)
                theta, phi = fsolve(null_U_entry, np.ones(2))  # type: ignore

                mat = mat @ U2block(dim, y, y+1, phi, theta).conj().T
                thetas[y, cnt_fore[y]] = theta
                phis[y, cnt_fore[y]] = phi
                cnt_fore[y] += 1
            else: #left multiply
                x = dim - 1 - p + q
                y = q

                def null_U_entry(angles):
                    return _U_times_BS_entry(angles, mat, x-1, x, x, y, is_odd = False)

                theta, phi = ( fsolve(null_U_entry, np.ones(2)))  # type: ignore

                mat = U2block(dim, x-1, x, phi, theta) @ mat
                thetas[x-1, cnt_back[x-1]] = theta
                phis[x-1, cnt_back[x-1]] = phi
                cnt_back[x-1] -= 1

    for p in range(dim-2, -1, -1): #move left multiply terms to the right
        for q in range(p, -1, -1):
            if p % 2 == 0:
                continue
            x = dim - 1 - p + q

            cnt_back[x-1] += 1
            theta = thetas[x-1, cnt_back[x-1]]
            phi = phis[x-1, cnt_back[x-1]]

            
            mat = U2block(dim, x-1, x, phi, theta).conj().T @ mat
            def null_U_entry(angles):
                return _U_times_BS_entry(angles, mat, x-1, x, x, x, is_odd = True)
            theta, phi = fsolve(null_U_entry, np.ones(2))  # type: ignore
            mat = mat @ U2block(dim, x-1, x, phi, theta).conj().T

            phis[x-1, cnt_back[x-1]] = phi
            thetas[x-1, cnt_back[x-1]] = theta
    for i in range(dim):
        alphas[i] = np.angle(mat[i, i])
        
    phis = np.remainder(phis, 2 * np.pi)
    thetas = np.remainder(thetas , np.pi)
    alphas = np.remainder(alphas, 2 * np.pi)
    return phis, thetas, alphas


def reconstruct_clements(phis, thetas, alphas, block='bs', Lp_dB=0, Lc_dB=0):
    assert len(phis.squeeze().shape) == 2 or phis.size == 1
    assert len(thetas.squeeze().shape) == 2 or thetas.size == 1
    assert len(alphas.squeeze().shape) == 1
    assert phis.squeeze().shape == thetas.squeeze().shape
    
    U2block = U2MZI3
    
    if thetas.size == 1:
        row = 1
        col = 1
    else:
        row, col = thetas.squeeze().shape
    dim = row + 1
    #num = int(dim * (dim - 1) / 2) 
    assert alphas.squeeze().shape[0] == dim
    
    sft = np.diag(np.exp(1j * alphas))
    mat = np.eye(dim)
    for p in range(col):
        for q in range(0, row, 2):
            #print('one', q, p)
            mat = U2block(dim, q, q+1, phis[q,p], thetas[q,p]) @ mat
        if p >= col - 1 and dim % 2 == 1:
            continue
        for q in range(1, row, 2):
            #print('two', q, p)
            mat = U2block(dim, q, q+1, phis[q,p], thetas[q,p]) @ mat
    mat = sft @ mat
    return mat

def flatten_phase(phis, thetas, alphas):
    assert len(phis.squeeze().shape) == 2 or phis.size == 1
    assert len(thetas.squeeze().shape) == 2 or thetas.size == 1
    assert len(alphas.squeeze().shape) == 1
    assert phis.squeeze().shape == thetas.squeeze().shape
    
    
    if thetas.size == 1:
        row = 1
        col = 1
    else:
        row, col = thetas.squeeze().shape
    dim = row + 1
    phases = np.zeros(0)
    #num = int(dim * (dim - 1) / 2) 
    assert alphas.squeeze().shape[0] == dim
    for p in range(col):
        odd = np.arange(0, row, 2)
        even = np.arange(1, row, 2)
        phases = np.append(phases, phis[odd, p])
        phases = np.append(phases, thetas[odd, p])

        #for q in range(0, row, 2):
            #print('one', q, p)
            #mat = U2block(dim, q, q+1, phis[q,p], thetas[q,p]) @ mat
        if p >= col - 1 and dim % 2 == 1:
            continue
        phases = np.append(phases, phis[even, p])
        phases = np.append(phases, thetas[even, p])
        #for q in range(1, row, 2):
            #print('two', q, p)
            #mat = U2block(dim, q, q+1, phis[q,p], thetas[q,p]) @ mat
            
    phases = np.append(phases, alphas)
    
    #phases = phases.reshape()
    
    return phases

def phases_to_voltages(phases, resolution):

    num_bytes = np.ceil(resolution / 8)
    #print(phases)

    tmp = phases / (2 * np.pi) 
    tmp = (tmp / (1 / (pow(2,resolution) - 1))).astype(int)
    #byte_array = []
    #tmp = tmp.flatten()
    #for i in range(len(tmp)):
    #    byte_array.append(list( bytearray( int(tmp.flatten()[i]).to_bytes(num_bytes, 'big'))))

    #print(tmp, byte_array)
    return tmp
    #return byte_array
def voltages_to_phases(voltages, resolution):

    return 2 * np.pi * (1 / (pow(2, resolution) - 1)) * voltages

def voltages_to_AD_levels(voltages, resolution, vmax):

    #return ((voltages / vmax) / (1 / (pow(2,resolution) - 1))).astype(int)
    return (voltages * (pow(2, resolution) - 1) / vmax).astype(int)

def AD_levels_to_voltages(levels, resolution, vmax):

    #return levels * (1 / (pow(2, resolution) - 1) * vmax)
    return vmax * levels /(pow(2, resolution) - 1)

def vector_to_voltages(vector, R, p_pi):

    return np.sqrt(np.arccos(vector) * (R * p_pi) / np.pi )

    #np.sqrt(np.arccos(vector) * (R * p_pi) / np.pi ) = x
    #x**2 = arccos()
    #return np.arccos(np.sqrt(vector)) * v_pi * 2 / np.pi

def voltages_to_vector(voltages, R, p_pi):

    return np.cos(voltages**2 * np.pi / (R * p_pi))**2
    #return np.cos((np.pi / 2) * voltages / v_pi)**2


def voltages_to_bytes(voltages, resolution):

    voltages = voltages.astype(int)
    #print(voltages, int(voltages[0]))

    num_bytes = int(np.ceil(resolution / 8))
    #print('num bytes: ', num_bytes, len(voltages))
    byte_array = []
    for i in range(len(voltages)):
        byte_array.append(list( bytearray( int(voltages[i]).to_bytes(num_bytes, 'little'))))
        #print(byte_array)
    #print(len(byte_array))
    return byte_array

    