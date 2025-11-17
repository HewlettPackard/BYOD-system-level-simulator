import numpy as np
from scipy.stats import unitary_group
from scipy.optimize import fsolve


class DAC():
    def __init__(self, resolution = 8, maxVin = 1.0):
        self.resolution = resolution
        self.maxVin = maxVin

    def getLevelsFromVoltages(self, voltages):
        return (voltages * (pow(2, self.resolution) - 1) / self.maxVin).astype(int)
    
    def getControllerEnergy(self): #TODO!!!
        return 0.0

class Modulator():
    def __init__(self, R = 0, pPi = 0):
        self.R = R
        self.p_pi = pPi

    def getVoltagesFromAmplitudes(self, data):
        return np.sqrt(np.arccos(data) * (self.R * self.p_pi) / np.pi)
    
    def getPhasesFromVoltages(self, voltages):
        return voltages**2 * np.pi / self.R
    
    def getVoltagesFromPhases(self, data):
        return np.sqrt(data * (self.R * self.p_pi) / np.pi)

class StreamingCPU():
    def __init__(self, resolution = 8):
        self.resolution = resolution

    def getBytesFromLevels(self, data):
        data = data.astype(int)
        num_bytes = int(np.ceil(self.resolution / 8))
        byte_array = []
        for i in range(len(data)):
            byte_array.append(list( bytearray( int(data[i]).to_bytes(num_bytes, 'little'))))
        return byte_array
    
class ADC():
    def __init__(self, resolution = 8):
        self.resolution = resolution

    def getConversionEnergy(self, data): #TODO!!!
        return 0.0
    
class ClementsMesh(Modulator):
    """
    Cast a magical spell using a wand and incantation.
    This function simulates casting a spell. With no
    target specified, it is cast into the void.

    :param wand: The wand used to do the spell-casting deed.
    :type wand: str
    :param incantation: The words said to activate the magic.
    :type incantation: str
    :param target: The object or person the spell is directed at (optional).
    :return: A string describing the result of the spell.
    :rtype: str

    """
    def __init__(self, R = 1000, pPi = 0.001, size = 8):
        Modulator.__init__(self, R = R, pPi= pPi)
        self.size = size
        self.voltages = np.zeros(self.size)

    def U2MZI(self, dim, m, n, phi, theta, Lp=1, Lc=1):
        assert m < n < dim
        mat = np.eye(dim, dtype=np.complex128)
        mat[m, m] = np.sqrt(Lp) * 1j * np.exp(1j * theta) * np.exp(1j * phi) * np.sin(theta)
        mat[m, n] = np.sqrt(Lc) * 1j * np.exp(1j * theta) * np.cos(theta)
        mat[n, m] = np.sqrt(Lc) * 1j * np.exp(1j * theta) * np.exp(1j * phi) * np.cos(theta)
        mat[n, n] = -np.sqrt(Lp) * 1j * np.exp(1j * theta) * np.sin(theta)
        return mat
    
    def _U_times_BS_entry(
        self,
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

    def decompose_clements(self, u):
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
        U2block = self.U2MZI

        
        for p in range(dim-1):
            for q in range(p+1):
                if p % 2 == 0: #right multiply
                    x = dim - 1 - q
                    y = p - q

                    def null_U_entry(angles):
                        return self._U_times_BS_entry(angles, mat, y, y+1, x, y, is_odd = True)
                    
                    theta, phi = fsolve(null_U_entry, np.ones(2))  # type: ignore
                    mat = mat @ U2block(dim, y, y+1, phi, theta).conj().T
                    thetas[y, cnt_fore[y]] = theta
                    phis[y, cnt_fore[y]] = phi
                    cnt_fore[y] += 1
                else: #left multiply
                    x = dim - 1 - p + q
                    y = q

                    def null_U_entry(angles):
                        return self._U_times_BS_entry(angles, mat, x-1, x, x, y, is_odd = False)

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
                    return self._U_times_BS_entry(angles, mat, x-1, x, x, x, is_odd = True)
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

    def flatten_phase(self, phis, thetas, alphas):
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
            phases = np.append(phases, 2 * thetas[odd, p])

            #for q in range(0, row, 2):
                #print('one', q, p)
                #mat = U2block(dim, q, q+1, phis[q,p], thetas[q,p]) @ mat
                #phases = np.append(phases, phis[q, p])
                #phases = np.append(phases, thetas[q, p])

            if p >= col - 1 and dim % 2 == 1:
                continue
            phases = np.append(phases, phis[even, p])
            phases = np.append(phases, 2 * thetas[even, p])
            #for q in range(1, row, 2):
                #phases = np.append(phases, phis[q, p])
                #phases = np.append(phases, thetas[q, p])

                #print('two', q, p)
                #mat = U2block(dim, q, q+1, phis[q,p], thetas[q,p]) @ mat
                
        phases = np.append(phases, alphas)
        #phases_out = torch.zeros(1, dim * dim)
        #phases_out[0,:] = torch.from_numpy(phases)
            
        return phases, dim
    
    def decompose(self, u):
        
        phis, thetas, alphas = self.decompose_clements(u)
        #print(phis, thetas, alphas)
        phases, size = self.flatten_phase(phis, thetas, alphas)
        #voltages = self.phases_to_norm_voltages(phases)
        self.voltages = self.getVoltagesFromPhases(phases)
        self.size = size
